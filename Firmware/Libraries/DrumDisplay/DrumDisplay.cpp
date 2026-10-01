/**
 * @file DrumDisplay.cpp
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 *
 * Renderer (drawTape / drawFlag / per-place ease / per-cell clip) ported from the
 * bench-verified SH1106 prototype. The class adds three things the prototype lacked:
 * DCS-BIOS address→digit decode (onControlPacket), snap-then-settle (update), and
 * descriptor-driven geometry auto-fit (fitGeometry) that replaces the prototype's
 * hardcoded PX_PER_MM / COLW / GAP / FLAGW / CELLH constants.
 */

#ifdef ARDUINO_ARCH_STM32

#include "DrumDisplay.h"
#include <STM32Board.h>   // log() — descriptor rejection (#137)
#include <math.h>
#include <string.h>

namespace OpenSkyhawk {

// Tuning constants (mirror the prototype: 0.30 ease, ~60 fps gate).
static const float    EASE         = 0.30f;  // per-frame ease factor (0..1); lower = slower roll
static const uint32_t FRAME_MS     = 16;     // ~60 fps render gate
static const float    SETTLE_EPS   = 0.02f;  // |target-pos| below which a place is "settled"
static const float    SNAP_LANDING = 1.5f;   // on a big jump, land this many digits shy of target
static const float    PX_PER_MM    = 4.35f;  // nominal scale (bench value); auto-shrink corrects it

// Cell kinds stored in _cellKind[].
static const uint8_t KIND_DIGIT = 0;
static const uint8_t KIND_GLYPH = 1;
static const uint8_t KIND_FLAG  = 2;

static long pow10l(uint8_t n) {
    long r = 1;
    while (n--) r *= 10;
    return r;
}

// ── construction ──────────────────────────────────────────────────────────────

DrumDisplay::DrumDisplay(U8G2& oled, const DrumReadout& readout,
                         DrumFont font, float xOffsetMm, float yOffsetMm)
    : _oled(&oled), _r(&readout), _mux(nullptr), _channel(0), _wire(&Wire), _font(font),
      _xOffMm(xOffsetMm), _yOffMm(yOffsetMm),
      _target(0), _flagTarget(0), _dirty(false), _hasState(false),
      _flagPos(0.0f),
      _geomDirty(false), _colW(0), _cellH(0), _gap(0), _flagW(0), _cy(0),
      _nCells(0), _lastFrameMs(0) {
    for (uint8_t i = 0; i < 6; i++) _pos[i] = 0.0f;
}

DrumDisplay::DrumDisplay(U8G2& oled, const DrumReadout& readout, TwoWire& wire,
                         DrumFont font, float xOffsetMm, float yOffsetMm)
    : DrumDisplay(oled, readout, font, xOffsetMm, yOffsetMm) {
    _wire = &wire;
}

DrumDisplay::DrumDisplay(U8G2& oled, const DrumReadout& readout,
                         I2cMux& mux, uint8_t channel,
                         DrumFont font, float xOffsetMm, float yOffsetMm)
    : DrumDisplay(oled, readout, font, xOffsetMm, yOffsetMm) {
    _mux     = &mux;
    _channel = channel;
    _wire    = &mux.bus();   // keep _wire truthful: the trunk is the mux's, not the default
}

// ── decode helpers ────────────────────────────────────────────────────────────

// Map an exported value onto its band, then onto what the drum shows.
//
// DCS-BIOS itself does not segment anything: Module.valueConvert() maps the gauge's declared arg
// range linearly onto 0..65535 (with a small snap at either end) and stops there. The segmentation
// is the gauge's, so each source declares its own band count. A plain A-4E-C digit drum carries
// digit/10 — utils.lua jumpwheel() returns B/10, and (B+dd)/10 mid-roll — so 10 steps, with the
// roll fraction living inside the band. Selector drums differ: the ARC-51 50 kHz group runs 20
// steps of 5, and the 10 MHz group 20 steps offset by 22.
//
// The value is TRUNCATED on the way out of DCS, so a position that should sit exactly on a band
// edge arrives one LSB below it (digit 9 as 58981, which is 8.99992 of a band). BAND_EPS pulls it
// back — it is far below the resolution of a roll fraction, so it cannot promote a rolling drum
// into the next digit.
long DrumDisplay::decodeDigits(uint16_t value, const DrumSource& s) {
    const uint16_t m      = s.mask ? s.mask : 0xFFFF;
    const uint32_t masked = static_cast<uint32_t>(value & m);
    const uint16_t steps  = s.steps ? s.steps : static_cast<uint16_t>(pow10l(s.nDigits));
    if (steps == 0) return s.offset;

    long idx;
    if (m == 0xFFFF) {
        // A float output: defineFloat always takes a whole word, so the value is the gauge's arg
        // normalised onto 0..65535 and the band says where that lands.
        const float pos = static_cast<float>(masked) / 65535.0f
                          * static_cast<float>(steps) + BAND_EPS;
        idx = static_cast<long>(pos);
    } else {
        // A packed field: defineMultipositionSwitch and friends allocate a small INTEGER into a
        // bit field, so the field already IS the position index — only right-justify it. Scaling
        // it like a float would be wrong, and would also break a field that is not low-justified.
        uint16_t shift = 0;
        while (shift < 16 && ((m >> shift) & 1u) == 0u) shift++;
        idx = static_cast<long>(masked >> shift);
    }

    if (idx < 0)                            idx = 0;
    if (idx > static_cast<long>(steps) - 1) idx = steps - 1;   // arg pinned at full scale
    return idx * static_cast<long>(s.mul) + static_cast<long>(s.offset);
}

// ── onControlPacket — decode + splice + mark dirty; NEVER draws ────────────────

void DrumDisplay::onControlPacket(uint16_t controlId, uint16_t value) {
    if (!_descriptorOk) return;   // disabled readout: decode nothing, so there is nothing to draw

    // Flag source? (A source may be BOTH a digit and the flag — NAV hemisphere dual-role —
    // so this does not early-return; the digit loop below still runs for the same address.)
    if (_r->flag.enabled && controlId == _r->flag.address) {
        uint16_t m = _r->flag.mask ? _r->flag.mask : 0xFFFF;
        uint32_t masked = static_cast<uint32_t>(value & m);
        int nFaces = static_cast<int>(strlen(_r->flag.faces));
        if (nFaces < 1) nFaces = 1;
        // A flag is discrete, not a rolling tape, so it rounds to the nearest position instead of
        // taking the band it sits in. steps defaults to nFaces−1 (a full-scale flag); the ASN-41
        // hemisphere needs steps = 2 because nav.lua drives it at 0.0 / 0.5.
        const uint16_t steps = _r->flag.steps ? _r->flag.steps : static_cast<uint16_t>(nFaces - 1);
        long face;
        if (m == 0xFFFF) {
            face = lroundf(static_cast<float>(masked) / 65535.0f * static_cast<float>(steps));
        } else {
            uint16_t shift = 0;                       // packed field: already the position index
            while (shift < 16 && ((m >> shift) & 1u) == 0u) shift++;
            face = static_cast<long>(masked >> shift);
        }
        if (face < 0)          face = 0;
        if (face > nFaces - 1) face = nFaces - 1;
        if (!_hasState || face != _flagTarget) {
            _flagTarget = face;
            _dirty      = true;
            _hasState   = true;
        }
    }

    // Digit source(s)? Scan ALL sources, not just the first match — two sources may share one
    // address with different masks (e.g. ARC-51 10 MHz + 1 MHz both at 0x853a, mask-separated).
    for (uint8_t i = 0; i < _r->nSources; i++) {
        const DrumSource& s = _r->sources[i];
        if (controlId != s.address) continue;
        long part     = decodeDigits(value, s);
        long lo       = pow10l(s.place);                 // weight of the low column of this field
        long hi       = pow10l(s.place + s.nDigits);     // weight just above the field
        long keepHigh = (_target / hi) * hi;             // digits above the field
        long keepLow  = _target % lo;                    // digits below the field
        long spliced  = keepHigh + part * lo + keepLow;
        if (!_hasState || spliced != _target) {
            _target   = spliced;
            _dirty    = true;
            _hasState = true;
        }
    }
}

// ── i2cProbe / oledAddr — I2cHealth reachability contract ──────────────────────

bool DrumDisplay::i2cProbe() {
#ifdef DRUMDISPLAY_TEST
    _probeCount++;
    if (_probeOverride >= 0) { _fault = _probeOverride ? Fault::None : Fault::Device; return _probeOverride != 0; }
#endif
    if (!_mux) {                                       // direct-bus: probe the OLED on its own trunk
        _wire->beginTransmission(oledAddr());          // Wire unless a bus was passed to the ctor
        const bool ok = (_wire->endTransmission() == 0);
        _fault = ok ? Fault::None : Fault::Device;
        return ok;
    }
    // Muxed: FORCE-write the channel (uncached) so a mux reset / power-glitch is re-routed; a NAK on
    // that write means the mux itself is gone. Then probe the OLED on the now-selected branch.
    if (!_mux->select(_channel, /*force=*/true)) { _fault = Fault::Mux;    return false; }
    if (!_mux->deviceAcks(oledAddr()))           { _fault = Fault::Device; return false; }
    _fault = Fault::None;
    return true;
}

uint8_t DrumDisplay::oledAddr() const {
    return static_cast<uint8_t>(_oled->getU8x8()->i2c_address >> 1);  // U8g2 stores the 8-bit (shifted) addr
}

// ── configure — auto-fit geometry, blank ───────────────────────────────────────

// The descriptor is a hand-authored constant, so these are mistakes made once at the bench rather
// than runtime conditions — but the library only ever sees it by reference, so the check lives
// here. A readout that fails is disabled rather than drawn: _pos[6]/_cellX[MAX_CELLS] are fixed
// arrays, and a descriptor that overruns them writes past the end of the object.
bool DrumDisplay::descriptorValid() const {
    if (_r == nullptr) return false;

    if (_r->nDigits < 1 || _r->nDigits > 6) {
        STM32Board::log("[DRUM] nDigits out of range (1..6) — readout disabled");
        return false;
    }

    const uint8_t cells = (uint8_t)(_r->nDigits + _r->nGlyphs + (_r->flag.enabled ? 1 : 0));
    if (cells > MAX_CELLS) {
        STM32Board::log("[DRUM] too many visual cells for MAX_CELLS — readout disabled");
        return false;
    }

    for (uint8_t i = 0; i < _r->nSources; i++) {
        const DrumSource& s = _r->sources[i];
        if (s.nDigits < 1 || (uint16_t)(s.place + s.nDigits) > _r->nDigits) {
            STM32Board::log("[DRUM] source place+nDigits exceeds the readout — readout disabled");
            return false;
        }
        // The band must fit the columns it is spliced into: a 2-digit field cannot show 100, and a
        // mis-copied steps/mul/offset is exactly the kind of mistake that would otherwise surface
        // as a quietly wrong readout rather than a refusal.
        const long steps    = s.steps ? (long)s.steps : pow10l(s.nDigits);
        const long maxShown = (steps - 1) * (long)s.mul + (long)s.offset;
        if (steps < 1 || s.offset < 0 || maxShown > pow10l(s.nDigits) - 1) {
            STM32Board::log("[DRUM] source band does not fit its digit columns — readout disabled");
            return false;
        }
    }
    return true;
}

void DrumDisplay::configure() {
    _descriptorOk = descriptorValid();
    if (!_descriptorOk) return;      // nothing is laid out, so nothing may be drawn

    _oled->setFont(fontPtr());
    _oled->setFontPosCenter();
    fitGeometry();                       // geometry from the U8G2 buffer dims — no I2C
    _oled->clearBuffer();
    if (i2cReachable()) {                // blank the panel; skip + trip the breaker if it's absent at boot
        _oled->sendBuffer();
#ifdef DRUMDISPLAY_TEST
        _renderCount++;
#endif
    }
}

// ── fitGeometry — descriptor mm + font + offset → px (replaces hardcoded consts) ─

void DrumDisplay::fitGeometry() {
    const int W = _oled->getDisplayWidth();
    const int H = _oled->getDisplayHeight();

    int colW  = lroundf(_r->digitWidthMm    * PX_PER_MM);
    int gap   = lroundf(_r->interDigitGapMm * PX_PER_MM);
    int grpG  = lroundf(_r->groupGapMm      * PX_PER_MM);
    int cellH = lroundf(_r->digitHeightMm   * PX_PER_MM);
    int flagW = _r->flag.enabled ? lroundf(_r->flag.widthMm * PX_PER_MM) : 0;

    // Build the ordered visual-cell list left→right: digits (leftmost = highest place),
    // glyphs at their afterCol, the flag at its atVisualCol. extraGap[] carries group gaps.
    uint8_t kinds[MAX_CELLS];
    int16_t datas[MAX_CELLS];
    int16_t widths[MAX_CELLS];
    int16_t extraGap[MAX_CELLS];
    uint8_t n = 0;

    for (uint8_t c = 0; c <= _r->nDigits && n < MAX_CELLS; c++) {
        for (uint8_t g = 0; g < _r->nGlyphs && n < MAX_CELLS; g++) {
            if (_r->glyphs[g].afterCol == c) {
                kinds[n] = KIND_GLYPH;
                datas[n] = static_cast<int16_t>(g);
                widths[n] = lroundf(_r->glyphs[g].widthMm * PX_PER_MM);
                extraGap[n] = 0;
                n++;
            }
        }
        if (_r->flag.enabled && _r->flag.atVisualCol == c && n < MAX_CELLS) {
            kinds[n] = KIND_FLAG;
            datas[n] = 0;
            widths[n] = static_cast<int16_t>(flagW);
            extraGap[n] = 0;
            n++;
        }
        if (c < _r->nDigits && n < MAX_CELLS) {
            kinds[n] = KIND_DIGIT;
            datas[n] = static_cast<int16_t>(_r->nDigits - 1 - c);  // visual L→R, leftmost = top place
            widths[n] = static_cast<int16_t>(colW);
            bool grpBoundary = (_r->groupSize > 0 && c > 0 && (c % _r->groupSize) == 0);
            extraGap[n] = grpBoundary ? static_cast<int16_t>(grpG) : 0;
            n++;
        }
    }

    // Total laid-out width: cells + inter-cell gaps + group gaps.
    int totalW = 0;
    for (uint8_t i = 0; i < n; i++) {
        totalW += widths[i];
        if (i > 0) totalW += gap;
        totalW += extraGap[i];
    }

    // Auto-shrink if the row is wider than the panel (the prototype's "won't fit" note, real).
    if (totalW > W && totalW > 0) {
        float k = static_cast<float>(W) / static_cast<float>(totalW);
        colW = static_cast<int>(floorf(colW * k));
        gap  = static_cast<int>(floorf(gap * k));
        flagW = static_cast<int>(floorf(flagW * k));
        for (uint8_t i = 0; i < n; i++) {
            widths[i]   = static_cast<int16_t>(floorf(widths[i] * k));
            extraGap[i] = static_cast<int16_t>(floorf(extraGap[i] * k));
        }
        totalW = 0;
        for (uint8_t i = 0; i < n; i++) {
            totalW += widths[i];
            if (i > 0) totalW += gap;
            totalW += extraGap[i];
        }
    }
    if (cellH > H) cellH = H;  // clamp roll window to short panels (128x32)

    int x0 = (W - totalW) / 2 + lroundf(_xOffMm * PX_PER_MM);  // centre the row, then apply the mm offset
    _cy = static_cast<int16_t>(H / 2 + lroundf(_yOffMm * PX_PER_MM));

    int x = x0;
    for (uint8_t i = 0; i < n; i++) {
        if (i > 0) x += gap;
        x += extraGap[i];
        _cellX[i]    = static_cast<int16_t>(x);
        _cellW[i]    = widths[i];
        _cellKind[i] = kinds[i];
        _cellData[i] = datas[i];
        x += widths[i];
    }

    _nCells = n;
    _colW   = static_cast<int16_t>(colW);
    _cellH  = static_cast<int16_t>(cellH);
    _gap    = static_cast<int16_t>(gap);
    _flagW  = static_cast<int16_t>(flagW);
    _geomDirty = false;
}

// ── ported renderers (cell width passed in, not a global) ──────────────────────

void DrumDisplay::drawTape(int16_t cx, float p, int16_t w) {
    long c = lroundf(p);
    for (long k = c - 1; k <= c + 1; k++) {
        int glyph = static_cast<int>(((k % 10) + 10) % 10);
        char s[2] = { static_cast<char>('0' + glyph), 0 };
        int gx = cx + (w - static_cast<int>(_oled->getStrWidth(s))) / 2;
        int y  = _cy + static_cast<int>(lroundf((static_cast<float>(k) - p) * _cellH));
        _oled->drawStr(gx, y, s);
    }
}

void DrumDisplay::drawFlag(int16_t cx, float p, int16_t w) {
    int nF = static_cast<int>(strlen(_r->flag.faces));
    if (nF < 1) return;
    long c = lroundf(p);
    for (long k = c - 1; k <= c + 1; k++) {
        int idx = static_cast<int>(((k % nF) + nF) % nF);
        char s[2] = { _r->flag.faces[idx], 0 };
        int gx = cx + (w - static_cast<int>(_oled->getStrWidth(s))) / 2;
        int y  = _cy + static_cast<int>(lroundf((static_cast<float>(k) - p) * _cellH));
        _oled->drawStr(gx, y, s);
    }
}

// ── update — frame gate + idle skip + ease/snap + render ───────────────────────

void DrumDisplay::update() {
    if (!_descriptorOk) return;                   // descriptor out of bounds → never lay out or draw
    if (!_hasState) return;                       // nothing received yet → stay blank
    uint32_t now = millis();
    if (now - _lastFrameMs < FRAME_MS) return;    // ~60 fps gate
    if (settled() && !_dirty && !_geomDirty) return;  // idle skip: no I2C when nothing moves
                                                      // (_geomDirty forces a frame after setOffset/setFontSize)
    _lastFrameMs = now;

    // Skip the render if the panel/mux is unreachable — keeps _dirty + tape positions, so the next
    // reachable frame catches up to the live value (no stale freeze). Probes ~every 2 s while dead.
    if (!i2cReachable()) return;

    if (_geomDirty) {
        _oled->setFont(fontPtr());
        _oled->setFontPosCenter();
        fitGeometry();
    }

    // Ease each place toward target/10^place, with SNAP_SETTLE jump handling.
    long place = 1;
    for (uint8_t i = 0; i < _r->nDigits; i++) {
        float step = static_cast<float>(_target / place);
        if (_r->scroll == DrumScroll::SNAP_SETTLE && fabsf(step - _pos[i]) > _r->snapThreshold) {
            _pos[i] = step - copysignf(SNAP_LANDING, step - _pos[i]);  // land just shy, same direction
        }
        _pos[i] += (step - _pos[i]) * EASE;
        place *= 10;
    }
    if (_r->flag.enabled) {
        float ft = static_cast<float>(_flagTarget);
        if (_r->scroll == DrumScroll::SNAP_SETTLE && fabsf(ft - _flagPos) > 1.0f) {
            _flagPos = ft - copysignf(0.9f, ft - _flagPos);
        }
        _flagPos += (ft - _flagPos) * EASE;
    }
    _dirty = false;

    // Render (select mux first; full-buffer; per-cell clip; one sendBuffer).
    if (_mux) _mux->select(_channel);
    _oled->clearBuffer();
    _oled->setFont(fontPtr());
    _oled->setFontPosCenter();
    const uint8_t vis = visibleDigits();  // low-order cells to draw; leading zeros above this stay blank
    for (uint8_t ci = 0; ci < _nCells; ci++) {
        int16_t cx = _cellX[ci];
        int16_t w  = _cellW[ci];
        int clipX0, clipY0, clipX1, clipY1;
        if (!clipRectFor(ci, clipX0, clipY0, clipX1, clipY1)) continue;  // entirely off-panel
        _oled->setClipWindow(static_cast<u8g2_uint_t>(clipX0), static_cast<u8g2_uint_t>(clipY0),
                             static_cast<u8g2_uint_t>(clipX1), static_cast<u8g2_uint_t>(clipY1));
        if (_cellKind[ci] == KIND_DIGIT) {
            const uint8_t place = static_cast<uint8_t>(_cellData[ci]);
            if (place < vis) drawTape(cx, _pos[place], w);  // else leading-zero cell → left blank
        } else if (_cellKind[ci] == KIND_GLYPH) {
            const DrumGlyph& g = _r->glyphs[_cellData[ci]];
            char s[2] = { g.ch, 0 };
            int gx = cx + (w - static_cast<int>(_oled->getStrWidth(s))) / 2;
            _oled->drawStr(gx, _cy, s);  // static glyph: centred, no tape
        } else {  // KIND_FLAG
            drawFlag(cx, _flagPos, w);
        }
    }
    _oled->setMaxClipWindow();
#ifdef DRUMDISPLAY_TEST
    _renderCount++;
#endif
    _oled->sendBuffer();  // the one expensive I2C op
}

// ── runtime setters ────────────────────────────────────────────────────────────

// Clip rect for one cell, clamped to the panel.
//
// u8g2's coordinates are UNSIGNED (u8g2_uint_t), so an edge that computes negative wraps to 65535
// and setClipWindow produces an EMPTY window — the readout vanishes instead of being clipped. The
// rect is built from _cy and _cellH, so a setOffset() nudge, or a tall cell on a short panel, is
// enough to push the top edge below zero.
//
// @return false when the cell lies entirely off the panel and must be skipped.
bool DrumDisplay::clipRectFor(uint8_t ci, int& x0, int& y0, int& x1, int& y1) {
    const int panelW = static_cast<int>(_oled->getDisplayWidth());
    const int panelH = static_cast<int>(_oled->getDisplayHeight());

    x0 = _cellX[ci];               y0 = _cy - _cellH / 2;
    x1 = _cellX[ci] + _cellW[ci];  y1 = _cy + _cellH / 2;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > panelW) x1 = panelW;
    if (y1 > panelH) y1 = panelH;
    return x1 > x0 && y1 > y0;
}

#ifdef DRUMDISPLAY_TEST
void DrumDisplay::debugDumpGeometry(Print& out) {
    if (_geomDirty || _nCells == 0) {
        _oled->setFont(fontPtr());
        _oled->setFontPosCenter();
        fitGeometry();
    }
    out.print(F("  panel ")); out.print(_oled->getDisplayWidth());
    out.print('x');           out.print(_oled->getDisplayHeight());
    out.print(F("  cy="));    out.print(_cy);
    out.print(F(" cellH="));  out.print(_cellH);
    out.print(F(" cells="));  out.print(_nCells);
    out.print(F(" descOk=")); out.print(_descriptorOk ? 1 : 0);
    out.print(F(" target=")); out.print(_target);
    out.print(F(" hasState=")); out.print(_hasState ? 1 : 0);
    // How much ink is actually in the full buffer after the last render. Geometry can look right
    // while nothing reaches the glass, so count the bytes rather than reason about it.
    {
        const uint8_t* buf = _oled->getBufferPtr();
        const uint16_t len = static_cast<uint16_t>(_oled->getBufferTileHeight()) * 8u
                           * static_cast<uint16_t>(_oled->getBufferTileWidth());
        uint16_t ink = 0;
        if (buf) for (uint16_t i = 0; i < len; i++) if (buf[i]) ink++;
        out.print(F(" inkBytes=")); out.print(ink);
        out.print('/');             out.println(len);
    }
    for (uint8_t ci = 0; ci < _nCells; ci++) {
        int x0, y0, x1, y1;
        const bool vis = clipRectFor(ci, x0, y0, x1, y1);
        out.print(F("   cell ")); out.print(ci);
        out.print(F(" kind "));   out.print(_cellKind[ci]);
        out.print(F(" x="));      out.print(_cellX[ci]);
        out.print(F(" w="));      out.print(_cellW[ci]);
        out.print(F(" clip("));   out.print(x0); out.print(',');  out.print(y0);
        out.print(F(")-("));      out.print(x1); out.print(',');  out.print(y1);
        out.print(F(") visible=")); out.println(vis ? 1 : 0);
    }
}

// Every visible cell must produce a clip rect that is non-empty and inside the panel. Without the
// clamp above, an offset readout returns a negative edge here — which on the panel is a blank
// screen, not a clipped one.
bool DrumDisplay::debugClipFits() {
    if (_geomDirty || _nCells == 0) {
        _oled->setFont(fontPtr());
        _oled->setFontPosCenter();
        fitGeometry();
    }
    const int panelW = static_cast<int>(_oled->getDisplayWidth());
    const int panelH = static_cast<int>(_oled->getDisplayHeight());
    bool anyVisible = false;
    for (uint8_t ci = 0; ci < _nCells; ci++) {
        int x0, y0, x1, y1;
        if (!clipRectFor(ci, x0, y0, x1, y1)) continue;
        anyVisible = true;
        if (x0 < 0 || y0 < 0 || x1 > panelW || y1 > panelH || x1 <= x0 || y1 <= y0) return false;
    }
    return anyVisible;
}
#endif

void DrumDisplay::setFontSize(DrumFont font) {
    _font      = font;
    _geomDirty = true;
}

void DrumDisplay::setOffset(float xOffsetMm, float yOffsetMm) {
    _xOffMm    = xOffsetMm;
    _yOffMm    = yOffsetMm;
    _geomDirty = true;
}

// ── small helpers ──────────────────────────────────────────────────────────────

bool DrumDisplay::settled() const {
    long place = 1;
    for (uint8_t i = 0; i < _r->nDigits; i++) {
        if (fabsf(static_cast<float>(_target / place) - _pos[i]) > SETTLE_EPS) return false;
        place *= 10;
    }
    if (_r->flag.enabled && fabsf(static_cast<float>(_flagTarget) - _flagPos) > SETTLE_EPS) return false;
    return true;
}

// Number of low-order digit cells to actually draw. With LeadingZero::Suppress the high-order
// zero cells are blanked down to the significant-digit count of the current target (min 1, so
// zero still shows a single "0"). Keep (default) → every cell draws, preserving fixed width.
uint8_t DrumDisplay::visibleDigits() const {
    if (_r->leadingZero == LeadingZero::Keep) return _r->nDigits;
    uint8_t nSig = 1;
    long t = _target < 0 ? -_target : _target;
    while (t >= 10 && nSig < _r->nDigits) { t /= 10; nSig++; }
    return nSig;
}

const uint8_t* DrumDisplay::fontPtr() const {
    return _font == DrumFont::LARGE ? u8g2_font_profont29_mr : u8g2_font_profont22_mr;
}

#ifdef DRUMDISPLAY_TEST
int16_t DrumDisplay::debugRowWidth() const {
    if (_nCells == 0) return 0;
    return static_cast<int16_t>(_cellX[_nCells - 1] + _cellW[_nCells - 1] - _cellX[0]);
}
#endif

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
