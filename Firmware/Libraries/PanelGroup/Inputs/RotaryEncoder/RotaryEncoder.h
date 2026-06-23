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

/** @brief Quadrature transitions per mechanical detent (match to the encoder). */
enum StepsPerDetent : uint8_t {
    ONE_STEP_PER_DETENT    = 1,
    TWO_STEPS_PER_DETENT   = 2,
    FOUR_STEPS_PER_DETENT  = 4,
    EIGHT_STEPS_PER_DETENT = 8,
};

/**
 * @brief Incremental quadrature encoder on two pins (A/B). Emits a **direction** per detent over
 *        CAN (ENCODER dispatch): 1 = clockwise, 0 = counter-clockwise. Self-registers into
 *        PanelGroup's InputBase list.
 *
 * @details A *relative* control — it reports motion, not an absolute position. Ports DcsBios
 * `RotaryEncoder`: each poll reads the 2-bit Gray state `(A<<1)|B`, a transition table accumulates
 * a signed delta, and when `|delta| >= stepsPerDetent` a CW (1) or CCW (0) EVT is emitted and the
 * delta is reduced by one detent. `stepsPerDetent` sets how many quadrature transitions make one
 * emitted click — set it to the encoder's transitions-per-detent so one physical click = one EVT.
 *
 * PanelBridge maps the 0/1 direction to the control's DCS-BIOS argument strings (`"DEC"`/`"INC"`
 * for fixed_step, the ± increment for variable_step) via the input map — so the same class drives
 * both kinds, the difference is entirely in the map entry.
 *
 * forceReport() resyncs the last state and emits **nothing** — a relative control has no baseline
 * to report at boot / SYNC. configure() does not enable internal pull-ups; the schematic biases
 * both pins (external pull-ups; the encoder commons to GND).
 *
 * Used by: AN/ASN-41 ×7 push-to-set knobs (variable_step), AN/ARC-51A ×4 freq/preset (fixed_step).
 */
class RotaryEncoder : public InputBase {
public:
    /**
     * @brief Construct a quadrature encoder.
     *
     * @param controlId       DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pinA            quadrature channel A.
     * @param pinB            quadrature channel B (swap A/B to reverse the sensed direction).
     * @param stepsPerDetent  quadrature transitions per emitted click (default ONE).
     */
    RotaryEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                  StepsPerDetent stepsPerDetent = ONE_STEP_PER_DETENT);

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
    /** @brief Test seam — last emitted direction (1 = CW, 0 = CCW, -1 = none). */
    int8_t lastDir() const { return _lastDir; }
#endif

protected:
    uint8_t readState();   ///< (pinA << 1) | pinB → 0..3.

private:
    void decode(uint8_t state);          ///< quadrature transition → delta → emit on detent.
    void emit(uint16_t dir);             ///< sendBatched an ENCODER EVT {controlId, dir}.

    uint16_t _controlId;
    PinRef   _pinA;
    PinRef   _pinB;
    uint8_t  _stepsPerDetent;
    uint8_t  _lastState;     ///< last 2-bit Gray state.
    int8_t   _delta;         ///< accumulated quadrature steps since the last emit.
    bool     _initialized;   ///< false until forceReport(); poll() no-op before this.
#ifdef ROTARYENCODER_TEST
    uint16_t _emitCount = 0;
    int8_t   _lastDir   = -1;
#endif
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
