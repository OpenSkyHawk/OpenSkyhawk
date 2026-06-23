/**
 * @file RotaryEncoder.h
 * @brief Quadrature rotary encoder input for OpenSkyhawk PanelGroup nodes.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // InputBase, PinRef

namespace OpenSkyhawk {

/** @brief Quadrature transitions per mechanical detent (match to the encoder). Scoped enum. */
enum class EncoderStepsPerDetent : uint8_t {
    One   = 1,
    Two   = 2,
    Four  = 4,
    Eight = 8,
};

/**
 * @brief Relative-dispatch mode — picks the DCS-BIOS interface the bridge drives, hence the CAN
 *        frame + payload encoding this encoder uses per detent. Scoped enum.
 */
enum class EncoderMode : uint8_t {
    Rel,  ///< variable_step knob: emit signed ±step on canIdEvtRel; bridge formats `%+d`.
    Dir,  ///< fixed_step selector: emit signed ±1 on canIdEvtDir; bridge formats `INC`/`DEC`.
};

/**
 * @brief Incremental quadrature encoder on two pins (A/B). Emits a signed **relative** value per
 *        detent over CAN — direction in the sign, magnitude set by the mode. Self-registers into
 *        PanelGroup's InputBase list.
 *
 * @details A *relative* control — it reports motion, not an absolute position. Ports DcsBios
 * `RotaryEncoder`: each poll reads the 2-bit Gray state `(A<<1)|B`, a transition table accumulates
 * a signed delta, and when `|delta| >= stepsPerDetent` one detent fires (CW positive / CCW negative)
 * and the delta is reduced by one detent. `stepsPerDetent` sets how many quadrature transitions make
 * one emitted click — set it to the encoder's transitions-per-detent so one physical click = one EVT.
 *
 * Two modes, chosen at construction (see EncoderMode):
 * - **REL** (variable_step knob, e.g. ASN-41 nav): emits `±step` on `canIdEvtRel`; the bridge sends
 *   `%+d` (e.g. `"+3200"`). `step` is build-side feel (default 3200 ≈ DCS suggested_step, ~20
 *   detents per full throw); lower it for a finer knob. The magnitude lives here on the node, so
 *   retuning needs only a node reflash — never a bridge rebuild.
 * - **DIR** (fixed_step selector with no indicator, e.g. ARC-51 freq): emits `±1` on `canIdEvtDir`;
 *   the bridge sends `INC`/`DEC`. Stateless — DCS owns the position and clamps at the band edges.
 *
 * Both modes are preset-safe: forceReport() resyncs the last Gray state and emits **nothing** — a
 * relative control has no baseline to assert at boot / SYNC, so it never clobbers a mission preset.
 * configure() does not enable internal pull-ups; the schematic biases both pins (external pull-ups;
 * the encoder commons to GND).
 *
 * Dispatch is sourced from the class (the CAN frame), not the input map — see #147.
 */
class RotaryEncoder : public InputBase {
public:
    static constexpr int16_t DEFAULT_STEP = 3200;  ///< REL per-detent magnitude (DCS suggested_step).

    /**
     * @brief Construct a quadrature encoder.
     *
     * @param controlId       DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pinA            quadrature channel A.
     * @param pinB            quadrature channel B (swap A/B to reverse the sensed direction).
     * @param stepsPerDetent  quadrature transitions per emitted click (default One; match the encoder).
     * @param mode            EncoderMode::Rel (variable_step, ±step) or ::Dir (fixed_step, ±1). Default Rel.
     * @param step            REL magnitude emitted per detent (default DEFAULT_STEP). Ignored in DIR.
     */
    RotaryEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                  EncoderStepsPerDetent stepsPerDetent = EncoderStepsPerDetent::One,
                  EncoderMode mode = EncoderMode::Rel, int16_t step = DEFAULT_STEP);

    /** @brief Read the quadrature state, accumulate, emit a direction once a detent completes. */
    void poll() override;

    /** @brief Resync the last state; emit nothing (relative control — no baseline). */
    void forceReport() override;

    /** @brief Configure both pins as inputs. Called by PanelGroup::setup(). */
    void configure() override;

#ifdef ROTARYENCODER_TEST
    /** @brief Test seam — set the starting Gray state (0..3) + clear the delta; no EVT. */
    void debugSeed(uint8_t state) { _lastState = (uint8_t)(state & 0x3); _delta = 0; _initialized = true; }
    /** @brief Test seam — feed the next 2-bit A/B Gray state through the decoder. */
    void debugStep(uint8_t ab) { decode((uint8_t)(ab & 0x3)); }
    /** @brief Test seam — count of CAN EVTs emitted. */
    uint16_t emitCount() const { return _emitCount; }
    /** @brief Test seam — last emitted signed value (REL: ±step; DIR: ±1; 0 = none yet). */
    int16_t lastValue() const { return _lastValue; }
    /** @brief Test seam — CAN frame id of the last emit (canIdEvtRel for REL, canIdEvtDir for DIR). */
    uint32_t lastFrame() const { return _lastFrame; }
#endif

protected:
    uint8_t readState();   ///< (pinA << 1) | pinB → 0..3.

private:
    void decode(uint8_t state);          ///< quadrature transition → delta → emit on detent.
    void emit(int8_t dir);               ///< emit one detent: dir = +1 (CW) / -1 (CCW).

    uint16_t   _controlId;
    PinRef     _pinA;
    PinRef     _pinB;
    EncoderMode _mode;
    int16_t    _step;        ///< REL magnitude per detent (unused in DIR).
    uint8_t    _stepsPerDetent;
    uint8_t    _lastState;   ///< last 2-bit Gray state.
    int8_t     _delta;       ///< accumulated quadrature steps since the last emit.
    bool       _initialized; ///< false until forceReport(); poll() no-op before this.
#ifdef ROTARYENCODER_TEST
    uint16_t _emitCount = 0;
    int16_t  _lastValue = 0;
    uint32_t _lastFrame = 0;
#endif
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
