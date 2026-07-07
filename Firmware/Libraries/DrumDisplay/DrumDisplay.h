/**
 * @file DrumDisplay.h
 * @brief Rolling mechanical-drum OLED readout output for OpenSkyhawk PanelGroup nodes.
 *
 * @details One DrumDisplay instance drives ONE OLED panel through a caller-owned U8G2
 * object. It renders a row of digit "tapes" that ease toward their target value with a
 * mechanical-drum cascade (units rolls continuously; higher places dwell and roll only on
 * carry), optionally followed by a 2-state hemisphere/mode flag tape. Multi-digit DCS-BIOS
 * readouts (APN-153 SPEED, NAV LAT/LON, MAGVAR, ARC-51 frequency, BDHI range) are
 * reconstructed from their per-digit DCS-BIOS output addresses via a DrumReadout descriptor.
 *
 * Renderer ported from the bench-verified SH1106 prototype (Tests/DrumDisplay roll_reference):
 * per-cell setClipWindow + drawTape/drawFlag + per-place ease. Adds snap-then-settle,
 * DCS-BIOS address→digit decode, and descriptor-driven geometry auto-fit.
 *
 * Ownership: the sketch constructs the concrete U8G2 type (it knows the panel — SH1106 vs
 * SSD1306, size, rotation), calls Wire.begin() and oled.begin(), and passes the U8G2 by
 * reference. DrumDisplay calls only base-class U8G2 methods. For N panels on one TCA9548A,
 * construct N DrumDisplay instances sharing one I2cMux, each with its own channel; the class
 * re-selects its channel before every buffer send.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>      // OutputBase
#include <NodeStatus.h>      // FaultSource + NodeFaultCode (#163)
#include <U8g2lib.h>
#include <Helpers/I2cMux/I2cMux.h>
#include <Helpers/I2cHealth/I2cHealth.h>

namespace OpenSkyhawk {

// ── Per-mounting font selection (ctor param, runtime-writable) ────────────────

/**
 * @brief Glyph font size. Maps to a fixed monospace ProFont face.
 * @note SMALL = u8g2_font_profont22_mr, LARGE = u8g2_font_profont29_mr (mono, ASCII).
 *       Mono guarantees the flag glyph ('N'/'S'/'E'/'W') is no wider than a digit cell.
 */
enum class DrumFont : uint8_t {
    SMALL = 0,  ///< u8g2_font_profont22_mr  (~22 px tall)
    LARGE = 1,  ///< u8g2_font_profont29_mr  (~29 px tall) — default
};

/**
 * @brief Scroll behaviour per readout.
 * @note SNAP_SETTLE adds the prototype-missing jump handling: deltas above the readout's
 *       snapThreshold teleport the tape near the target, then ease the final step, so a
 *       sudden 130→250 KIAS change doesn't spin every wheel through 120 intermediate values.
 */
enum class DrumScroll : uint8_t {
    EASE_ONLY   = 0,  ///< pure cascade ease (prototype behaviour)
    SNAP_SETTLE = 1,  ///< snap near target on large jumps, then ease the last bit — default
};

/**
 * @brief Leading-zero handling for a readout's high-order digit cells.
 * @note Suppress blanks the high-order zero cells down to the target's significant-digit count
 *       (units always shows, so 0 renders "0"); animation is unchanged. Keep is fixed width.
 */
enum class LeadingZero : uint8_t {
    Keep     = 0,  ///< render all nDigits — fixed width (default)
    Suppress = 1,  ///< blank leading zeros — variable width (e.g. ARC-51 preset channel 1..20)
};

// ── Descriptor structs (mm-based, resolution-independent) ─────────────────────

/**
 * @brief One DCS-BIOS digit source feeding a DrumReadout.
 * @note Populate @c address / @c mask from the A_4E_C_* and A_4E_C_*_AM constants in
 *       A4EC_OutputIds.h. The masked value (0..mask) is scaled to nDigits decimal digits:
 *       digits = round((value & mask) / mask * (10^nDigits − 1)); each of those nDigits
 *       digits is spliced into the readout's combined number starting at @c place
 *       (0 = least-significant column).
 */
struct DrumSource {
    uint16_t address;   ///< DCS-BIOS output address (A_4E_C_* from A4EC_OutputIds.h)
    uint16_t mask;      ///< field mask (A_4E_C_*_AM); 0xFFFF for whole-word sources
    uint8_t  nDigits;   ///< digits this address encodes (1 for APN153, 2 for ARC-51 groups)
    uint8_t  place;     ///< least-significant digit column this source writes (0 = rightmost)
};

/**
 * @brief A fixed (non-rolling) glyph painted between digit columns — '.', ' ', ':' etc.
 * @note Drawn statically (no tape) in its own cell. Used for the ARC-51 / altimeter decimal.
 */
struct DrumGlyph {
    char    ch;         ///< literal glyph to paint
    uint8_t afterCol;   ///< visual column it follows: inserted after this many digits from the left
    float   widthMm;    ///< cell width for this glyph, mm (narrower than a digit, e.g. 1.8 for '.')
};

/**
 * @brief Optional 2-state (or N-state) flag tape — hemisphere N/S · E/W, or a mode letter.
 * @note OFF by default (@c enabled = false). Position is configurable (not hardcoded
 *       rightmost). Populate @c address / @c mask from the A4EC constants. For a whole-word
 *       source (mask 0xFFFF) the value maps to a face by round(value/65535·(nFaces−1));
 *       for a bit-packed source the masked value selects face 0 (zero) or the last face
 *       (non-zero). Bench-confirm the real encoding before trusting it.
 */
struct DrumFlag {
    bool        enabled;      ///< false ⇒ no flag tape rendered (default)
    uint16_t    address;      ///< DCS-BIOS address that drives the flag
    uint16_t    mask;         ///< field mask (A_4E_C_*_AM); 0xFFFF for whole-word
    const char* faces;        ///< face string, one char per state, e.g. "NS" / "EW" (nFaces = strlen)
    uint8_t     atVisualCol;  ///< visual column the flag cell is inserted at (after this many digits)
    float       widthMm;      ///< flag cell width, mm (wider than a digit so a broad 'W' fits)
};

/**
 * @brief Complete description of one rolling readout: its sources, geometry, glyphs, flag.
 * @note All spatial fields are mm; DrumDisplay converts to px at configure() from the panel's
 *       getDisplayWidth()/getDisplayHeight(). @c digitWidthMm / @c digitHeightMm are the
 *       drum-window aperture. If the laid-out row is wider than the panel, auto-fit shrinks
 *       the pitch to the pixel width.
 */
struct DrumReadout {
    // ── required: sources + geometry (always set) ──
    const DrumSource* sources;        ///< array of digit sources
    uint8_t  nSources;                ///< element count of @c sources
    uint8_t  nDigits;                 ///< total digit columns in the combined number (1..6)
    float    digitWidthMm;            ///< digit cell (window aperture) width,  mm
    float    digitHeightMm;           ///< digit cell (roll window) height, mm
    float    interDigitGapMm;         ///< gap between adjacent cells, mm
    // ── optional: default-member-initialized, set only when non-default (use designated init) ──
    float    groupGapMm     = 0.0f;               ///< extra gap at group boundaries, mm (0 if ungrouped)
    uint8_t  groupSize      = 0;                  ///< digits per group for groupGap insertion (0 = no grouping)
    const DrumGlyph* glyphs = nullptr;            ///< fixed glyphs (decimal point etc.); nullptr if none
    uint8_t  nGlyphs        = 0;                  ///< element count of @c glyphs
    DrumFlag flag           = {};                 ///< optional flag ({} ⇒ disabled)
    DrumScroll scroll       = DrumScroll::SNAP_SETTLE;  ///< EASE_ONLY or SNAP_SETTLE (default)
    float    snapThreshold  = 3.0f;               ///< |target−pos| (digit units) above which SNAP_SETTLE teleports
    LeadingZero leadingZero = LeadingZero::Keep;  ///< Keep (default) = all nDigits; Suppress = variable width
};

// ── The output class ──────────────────────────────────────────────────────────

/**
 * @brief Rolling-drum OLED readout. One instance == one OLED panel.
 *
 * @details Construct at global scope so OutputBase self-registers it. onControlPacket()
 * only decodes + flags dirty (cheap); update() does the ~60 fps gate, channel reselect,
 * ease+snap, render, and the single expensive sendBuffer().
 */
class DrumDisplay : public OutputBase, public I2cHealth, public FaultSource {
public:
    /** @brief Which I2C hop failed the last reachability probe (feeds node health reporting, #163). */
    enum class Fault : uint8_t { None, Mux, Device };

    /**
     * @brief Construct and register a direct-bus drum display.
     * @param oled       Caller-owned U8G2 (already begin()'d, rotation set). Must outlive this.
     * @param readout    Descriptor for this readout (sources, geometry, flag). Must outlive this.
     * @param font       Per-mounting glyph size. Default DrumFont::LARGE.
     * @param xOffsetMm  X shift (mm) of the whole digit block, registers it to the faceplate window.
     * @param yOffsetMm  Y shift (mm) of the digit block centre line.
     * @note The sketch owns Wire.begin() + oled.begin(). Geometry is auto-fitted in configure().
     */
    DrumDisplay(U8G2& oled, const DrumReadout& readout,
                DrumFont font = DrumFont::LARGE,
                float xOffsetMm = 0.0f, float yOffsetMm = 0.0f);

    /**
     * @brief Construct and register a muxed drum display (one TCA9548A branch).
     * @param oled       Caller-owned U8G2. Must outlive this.
     * @param readout    Descriptor. Must outlive this.
     * @param mux        Shared TCA9548A selector. Must outlive this.
     * @param channel    TCA9548A channel 0–7 this panel sits on.
     * @param font       Per-mounting glyph size. Default DrumFont::LARGE.
     * @param xOffsetMm  X registration shift (mm).
     * @param yOffsetMm  Y registration shift (mm).
     * @note The class calls mux.select(channel) before geometry fit in configure() and before
     *       each sendBuffer() so interleaved displays never write to the wrong panel.
     */
    DrumDisplay(U8G2& oled, const DrumReadout& readout,
                I2cMux& mux, uint8_t channel,
                DrumFont font = DrumFont::LARGE,
                float xOffsetMm = 0.0f, float yOffsetMm = 0.0f);

    /**
     * @brief Compute pixel geometry from the panel + descriptor, set the font, blank the panel.
     * @note Called once by PanelGroup::setup() after chip init. Selects the mux channel (if any)
     *       first, reads getDisplayWidth()/Height(), runs the geometry fit, and clears the OLED.
     *       The sketch owns oled.begin(); this does NOT call begin().
     */
    void configure() override;

    /**
     * @brief Decode one CTRL_BCAST packet into this readout's digits/flag. Never draws.
     * @param controlId  DCS-BIOS output address from the packet. Ignored unless it matches a
     *                   DrumSource.address or the flag.address of this readout.
     * @param value      16-bit DCS-BIOS value (0..65535) for that address.
     * @note Cheap: decodes value→digit(s), splices into the target number, sets the dirty flag.
     *       The expensive full-buffer I2C send happens in update(), never here.
     */
    void onControlPacket(uint16_t controlId, uint16_t value) override;

    /**
     * @brief Advance the ease/snap animation and push one frame if needed.
     * @note Called every loop(). ~60 fps frame gate; early-out when idle (settled AND not dirty).
     *       On a live frame: mux.select(channel), ease each place toward target/10^place with
     *       SNAP_SETTLE jump handling, render every cell with setClipWindow, then one sendBuffer().
     */
    void update() override;

    /**
     * @brief Change glyph size at runtime (e.g. swap a cramped 6-digit readout to SMALL).
     * @param font  New DrumFont. Re-fits geometry on the next update() so cells re-auto-fit.
     */
    void setFontSize(DrumFont font);

    /**
     * @brief Re-register the digit block to the faceplate window at runtime.
     * @param xOffsetMm  New X shift (mm) of the digit block — measure the cutout misalignment after assembly.
     * @param yOffsetMm  New Y shift (mm, centre line) of the digit block.
     * @note Marks geometry dirty; the new offsets apply on the next rendered frame. Resolution-
     *       independent (converted to px via the panel scale), so the same mm value registers
     *       correctly on any OLED size.
     * @note Rounds to whole pixels, so the smallest useful step is ~0.25 mm (≈1 px); a 0.1 mm
     *       nudge is sub-pixel and may not move. The 1.3" 128x64 (~0.23 mm/px) is the coarser panel.
     */
    void setOffset(float xOffsetMm, float yOffsetMm);

    /**
     * @brief FaultSource: I2C_PERIPHERAL when the I2cHealth breaker is tripped, else NONE (#163).
     *        Cached breaker state only — no I2C op. The node aggregator packs this into HEALTH_n.faultId.
     */
    NodeFaultCode faultCode() const override {
        return i2cHealthy() ? NodeFaultCode::NONE : NodeFaultCode::I2C_PERIPHERAL;
    }

    /** @brief DiagSerial-only fault detail (#163): which I2C hop failed the last probe. */
    const char* faultDetail() const override {
        if (i2cHealthy()) return "";
        switch (_fault) {
            case Fault::Mux:    return "I2C mux unreachable";
            case Fault::Device: return "OLED not responding";
            default:            return "I2C peripheral fault";
        }
    }

#ifdef DRUMDISPLAY_TEST
    long    debugTarget() const     { return _target; }      ///< test-only: decoded combined number
    long    debugFlagTarget() const { return _flagTarget; }  ///< test-only: decoded flag face index
    uint8_t debugCellCount() const  { return _nCells; }      ///< test-only: laid-out visual cell count
    int16_t debugRowWidth() const;                           ///< test-only: total laid-out width, px
    uint8_t debugVisibleDigits() const { return visibleDigits(); }  ///< test-only: digit cells drawn (leading zeros blanked)
    int16_t debugCellX0() const     { return _nCells ? _cellX[0] : 0; }  ///< test-only: leftmost cell X, px
    bool     debugHealthy() const     { return i2cHealthy(); }                  ///< test-only: breaker state
    uint8_t  debugFault() const       { return static_cast<uint8_t>(_fault); }  ///< test-only: 0=None 1=Mux 2=Device
    uint32_t debugRenderCount() const { return _renderCount; }                  ///< test-only: sendBuffer() count
    void     debugForceProbe(int v)   { _probeOverride = v; }                   ///< test-only: -1 real / 0 fail / 1 ok
    bool     debugReachable()          { return i2cReachable(); }               ///< test-only: drive the breaker gate
    uint32_t debugProbeCount() const   { return _probeCount; }                  ///< test-only: i2cProbe() calls
#endif

protected:
    bool i2cProbe() override;  // I2cHealth contract: mux present + OLED ACKs on its channel; sets _fault

private:
    static constexpr uint8_t MAX_CELLS = 8;  // 6 digits + 1 glyph + 1 flag

    uint8_t oledAddr() const;        // OLED 7-bit address, read from the U8G2 object (for the probe)
    Fault   _fault = Fault::None;    // which hop failed the last probe (mux vs device)
#ifdef DRUMDISPLAY_TEST
    uint32_t _renderCount  = 0;      // sendBuffer() calls — render-skip assertion
    uint32_t _probeCount   = 0;      // i2cProbe() calls — back-off assertion
    int      _probeOverride = -1;    // -1 = real probe, 0 = force fail, 1 = force ok
#endif

    // collaborators / config (plain // — EXTRACT_PRIVATE NO, not rendered in API docs)
    U8G2*              _oled;        // caller-owned panel
    const DrumReadout* _r;           // descriptor (not owned)
    I2cMux*            _mux;          // nullptr for direct-bus instances
    uint8_t            _channel;      // mux channel; ignored when _mux == nullptr
    DrumFont           _font;         // current glyph size
    float              _xOffMm, _yOffMm;  // registration offset, mm (→ px via PX_PER_MM in fitGeometry)

    // decoded value state (set in onControlPacket, read in update)
    long               _target;       // combined integer the digit row should show
    long               _flagTarget;   // flag face index target (0..nFaces-1)
    volatile bool      _dirty;        // a source/flag changed since last render
    bool               _hasState;     // false until first matching packet (blank until then)

    // animation state (continuous tape positions, [0] = least-significant)
    float              _pos[6];       // up to 6 digit tapes; _r->nDigits used
    float              _flagPos;      // flag tape continuous position

    // geometry, auto-fitted in fitGeometry()
    bool               _geomDirty;    // recompute on next frame (font/offset changed)
    int16_t            _colW;         // digit cell width, px
    int16_t            _cellH;        // roll-window height, px
    int16_t            _gap;          // inter-cell gap, px
    int16_t            _flagW;        // flag cell width, px (0 if no flag)
    int16_t            _cy;           // row centre Y, px (display centre + yOff)
    int16_t            _cellX[MAX_CELLS];     // precomputed left-X of each visual cell, px
    int16_t            _cellW[MAX_CELLS];     // width of each visual cell, px
    uint8_t            _cellKind[MAX_CELLS];  // 0 = digit, 1 = glyph, 2 = flag
    int16_t            _cellData[MAX_CELLS];  // digit: place index; glyph: index into glyphs[]
    uint8_t            _nCells;       // total visual cells (digits + glyphs + flag)
    uint32_t           _lastFrameMs;  // ~60 fps gate timestamp

    // helpers
    void fitGeometry();                              // mm + font + offset → px, fill _cellX[]
    void drawTape(int16_t cx, float p, int16_t w);   // ported rolling digit tape
    void drawFlag(int16_t cx, float p, int16_t w);   // ported 2-face flag tape
    bool settled() const;                            // every |target/10^k − pos[k]| < epsilon
    uint8_t visibleDigits() const;                   // significant digit cells to draw (== nDigits unless suppressLeadingZero)
    const uint8_t* fontPtr() const;                  // ProFont face for _font
    static long decodeDigits(uint16_t value, uint16_t mask, uint8_t nDigits);
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
