/**
 * @file Switch3Pos.h
 * @brief Three-position (ON-OFF-ON) switch for OpenSkyhawk PanelGroup nodes.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>                          // PinRef
#include <Inputs/MultiPosInput/MultiPosInput.h>  // MultiPosInput base

namespace OpenSkyhawk {

/**
 * @brief Three-position switch (ON-OFF-ON / spring-centred) on two pins. Emits 0 / 1 / 2 over
 *        CAN (MULTIPOS dispatch).
 *
 * @details A MULTIPOS-family input (N = 3) — self-registers via MultiPosInput, which owns the
 * debounce / emit-on-change / forceReport contract. This class supplies only the read.
 *
 * Two pins, one per outer throw; the centre needs no pin of its own:
 *  - pin A active → position 0
 *  - pin B active → position 2
 *  - neither      → position 1 (centre — a real, emitted position)
 *
 * Mirrors DcsBios `Switch3Pos`: if both pins read active (impossible mechanically — a bounce
 * during a throw), pin A wins (position 0); the debounce absorbs the transient either way.
 * Because the centre is resolved directly, readRaw() never returns NO_POSITION — the base's
 * hold-last path is unused.
 *
 * One-hot read (reverse = false, default): the active pin reads LOW (closed to GND). reverse =
 * true inverts it (active pin reads HIGH). configure() does not enable internal pull-ups; the
 * schematic supplies input bias (typically 10 kΩ to +3.3V, switch to GND).
 *
 * Used by: AN/ASN-41 LAT/LON slew (spring-centred momentary L / centre / R).
 */
class Switch3Pos : public MultiPosInput {
public:
    static constexpr uint16_t DEBOUNCE_MS = 20;   ///< index stability window (ms).

    /**
     * @brief Construct a 3-position switch.
     *
     * @param controlId   DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pinA        outer throw → position 0 (active-LOW unless @p reverse).
     * @param pinB        outer throw → position 2.
     * @param reverse     false (default): active pin reads LOW. true: active pin reads HIGH.
     * @param debounceMs  index stability window before a change is confirmed (default 20 ms).
     */
    Switch3Pos(uint16_t controlId, PinRef pinA, PinRef pinB, bool reverse = false,
               uint16_t debounceMs = DEBOUNCE_MS);

    /**
     * @brief Configure both pins as inputs. Called by PanelGroup::setup().
     *
     * Does not enable internal pull-ups; board wiring supplies input bias.
     *
     * @note Must not run from the constructor — MCP23017 register writes require the expander
     * to be initialised first.
     */
    void configure() override;

protected:
    /** @brief pin A → 0, pin B → 2, neither → 1 (centre). pin A wins if both read active. */
    uint16_t readRaw() override;

private:
    PinRef _pinA;     ///< outer throw → position 0.
    PinRef _pinB;     ///< outer throw → position 2.
    bool   _reverse;  ///< true = active-HIGH.
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
