/**
 * @file RotaryAcceleratedEncoder.h
 * @brief Quadrature encoder with a momentum filter and a fast-detent step — DcsBios
 *        RotaryAcceleratedEncoder parity (D16, #287).
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Inputs/RotaryEncoder/RotaryEncoder.h>

namespace OpenSkyhawk {

/**
 * @brief `RotaryEncoder` plus the two behaviours of `DcsBios::RotaryAcceleratedEncoder`, both on by
 *        construction. Header-only: the logic lives in the RotaryEncoder family base.
 *
 * @details
 * - **Momentum filter** — the DCS-BIOS class's stated purpose ("noisy/faulty rotaries"). Momentum
 *   builds by one per transition in the current direction, up to ±(MAX_MOMENTUM × stepsPerDetent);
 *   a transition against it is dropped and only decays it; after STOPPED_THRESHOLD_MS (500 ms)
 *   with no detent it resets, so a deliberate reversal from rest registers normally.
 * - **Speed** (REL only) — a detent completing less than FAST_THRESHOLD_MS (175 ms) after the
 *   previous one sends @p fastStep instead of @p step. Two-tier, not a ramp. The first detent after
 *   a resync is always slow. A fast detent is just a bigger `±step` on the REL frame — no wire,
 *   bridge or input-map change (supersedes D9).
 *
 * DIR mode gets the filter only: the DIR frame carries ±1 and PanelBridge drops anything else,
 * so there is no speed to add — the way DCS-BIOS users apply the class to INC/DEC controls.
 */
class RotaryAcceleratedEncoder : public RotaryEncoder {
public:
    /**
     * @brief REL (variable_step) knob: momentum filter + speed-up.
     *
     * @param controlId       DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pinA            quadrature channel A.
     * @param pinB            quadrature channel B (swap A/B to reverse the sensed direction).
     * @param stepsPerDetent  quadrature transitions per emitted click (match the encoder).
     * @param step            REL magnitude for a normal detent (DCS suggested_step is 3200).
     * @param fastStep        REL magnitude for a fast detent, same sign as @p step (e.g. 4× step).
     *                        0 turns the speed-up off, leaving only the filter.
     */
    RotaryAcceleratedEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                             EncoderStepsPerDetent stepsPerDetent, int16_t step, int16_t fastStep)
        : RotaryEncoder(controlId, pinA, pinB, stepsPerDetent, EncoderMode::Rel, step,
                        /*momentumFilter*/ true, fastStep) {}

    /**
     * @brief Momentum filter only — for DIR (fixed_step) selectors.
     *
     * @param mode  EncoderMode::Dir. Passing EncoderMode::Rel here gives a REL knob with the filter
     *              and no speed-up (use the other constructor to set @p fastStep).
     */
    RotaryAcceleratedEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                             EncoderStepsPerDetent stepsPerDetent, EncoderMode mode)
        : RotaryEncoder(controlId, pinA, pinB, stepsPerDetent, mode, DEFAULT_STEP,
                        /*momentumFilter*/ true, /*fastStep*/ 0) {}
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
