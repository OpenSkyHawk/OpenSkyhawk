

# File DrumDisplay.cpp

[**File List**](files.md) **>** [**DrumDisplay**](dir_b8ac8c4bf654b399fff34fe5003d7d6c.md) **>** [**DrumDisplay.cpp**](DrumDisplay_8cpp.md)

[Go to the documentation of this file](DrumDisplay_8cpp.md)


```C++

#ifdef ARDUINO_ARCH_STM32

#include "DrumDisplay.h"
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
                         DrumFont font, int16_t xOffsetPx, int16_t yOffsetPx)
    : _oled(&oled), _r(&readout), _mux(nullptr), _channel(0), _font(font),
      _xOff(xOffsetPx), _yOff(yOffsetPx),
      _target(0), _flagTarget(0), _dirty(false), _hasState(false),
      _flagPos(0.0f),
      _geomDirty(false), _colW(0), _cellH(0), _gap(0), _flagW(0), _cy(0),
      _nCells(0), _lastFrameMs(0) {
    for (uint8_t i = 0; i < 6; i++) _pos[i] = 0.0f;
}

DrumDisplay::DrumDisplay(U8G2& oled, const DrumReadout& readout,
                         I2cMux& mux, uint8_t channel,
                         DrumFont font, int16_t xOffsetPx, int16_t yOffsetPx)
    : DrumDisplay(oled, readout, font, xOffsetPx, yOffsetPx) {
    _mux     = &mux;
    _channel = channel;
}

// ── decode helpers ────────────────────────────────────────────────────────────

long DrumDisplay::decodeDigits(uint16_t value, uint16_t mask, uint8_t nDigits) {
    uint16_t m = mask ? mask : 0xFFFF;
    uint32_t masked = static_cast<uint32_t>(value & m);
    long span = pow10l(nDigits) - 1;  // 1 digit → 9, 2 digits → 99
    // Whole-word sources (m == 0xFFFF) scale 0..65535 → 0..span. A low-justified bit field
    // scales 0..m → 0..span. Non-contiguous packed fields would need a right-shift first —
    // none shipped; see the TODO(bench) notes on the sketch-defined descriptors.
    return lroundf(static_cast<float>(masked) / static_cast<float>(m) * static_cast<float>(span));
}

// ── onControlPacket — decode + splice + mark dirty; NEVER draws ────────────────

void DrumDisplay::onControlPacket(uint16_t controlId, uint16_t value) {
    // Flag source? (A source may be BOTH a digit and the flag — NAV hemisphere dual-role —
    // so this does not early-return; the digit loop below still runs for the same address.)
    if (_r->flag.enabled && controlId == _r->flag.address) {
        uint16_t m = _r->flag.mask ? _r->flag.mask : 0xFFFF;
        uint32_t masked = static_cast<uint32_t>(value & m);
        int nFaces = static_cast<int>(strlen(_r->flag.faces));
        if (nFaces < 1) nFaces = 1;
        long face = (m == 0xFFFF)
                        ? lroundf(static_cast<float>(masked) / 65535.0f * static_cast<float>(nFaces - 1))
                        : (masked ? (nFaces - 1) : 0);
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
        long part     = decodeDigits(value, s.mask, s.nDigits);
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

// ── configure — auto-fit geometry, blank ───────────────────────────────────────

void DrumDisplay::configure() {
    if (_mux) _mux->select(_channel);
    _oled->setFont(fontPtr());
    _oled->setFontPosCenter();
    fitGeometry();
    _oled->clearBuffer();
    _oled->sendBuffer();  // blank until the first packet arrives (_hasState stays false)
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

    int x0 = (W - totalW) / 2 + _xOff;  // centre the row, then apply registration offset
    _cy = static_cast<int16_t>(H / 2 + _yOff);

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
    if (!_hasState) return;                       // nothing received yet → stay blank
    uint32_t now = millis();
    if (now - _lastFrameMs < FRAME_MS) return;    // ~60 fps gate
    if (settled() && !_dirty) return;             // idle skip: no I2C when nothing moves
    _lastFrameMs = now;

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
    for (uint8_t ci = 0; ci < _nCells; ci++) {
        int16_t cx = _cellX[ci];
        int16_t w  = _cellW[ci];
        _oled->setClipWindow(cx, _cy - _cellH / 2, cx + w, _cy + _cellH / 2);
        if (_cellKind[ci] == KIND_DIGIT) {
            drawTape(cx, _pos[_cellData[ci]], w);
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
    _oled->sendBuffer();  // the one expensive I2C op
}

// ── runtime setters ────────────────────────────────────────────────────────────

void DrumDisplay::setFontSize(DrumFont font) {
    _font      = font;
    _geomDirty = true;
}

void DrumDisplay::setOffset(int16_t xOffsetPx, int16_t yOffsetPx) {
    _xOff      = xOffsetPx;
    _yOff      = yOffsetPx;
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
```


