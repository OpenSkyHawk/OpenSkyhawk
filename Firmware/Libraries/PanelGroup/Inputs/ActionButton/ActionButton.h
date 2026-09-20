/**
 * @file ActionButton.h
 * @brief Momentary push button that fires one DCS-BIOS action per press.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // InputBase, PinRef

namespace OpenSkyhawk {

/**
 * @brief Momentary push button driving a control that latches in the sim. Self-registers into
 *        PanelGroup's InputBase list.
 *
 * @details Emits **one** EVT_ACTION frame on the press edge and nothing on release. The payload
 * value is a selector, not a magnitude: 0 means `TOGGLE`, which PanelBridge renders as the
 * DCS-BIOS action argument. The DCS-BIOS string never appears on this side of the bus.
 *
 * **What it is for.** `Switch2Pos` sends absolute 0/1, so it needs a physical switch that latches.
 * This class lets a *momentary* part drive a control that latches *in the sim*: DCS-BIOS reads the
 * control's current value, flips it, and writes it back, so each press toggles and the state
 * persists. Because the sim supplies the current value, the panel cannot desync from it — unlike a
 * physical latching switch, whose position can disagree with the sim after a cold start, a keyboard
 * binding, or a mission script. The trade-off is that a momentary button shows no state at a
 * glance.
 *
 * **Only for controls that latch in the sim** — DCS-BIOS `defineToggleSwitch`. Pointing this at a
 * `definePushButton` control is wrong: those are `momentary_last_position`, and a TOGGLE latches
 * them on until the next press. Use `Switch2Pos` for those.
 *
 * VALUE semantics (reverse = false, default):
 *   press (pin LOW — button closed, pulling pin to GND via board pull-up) — emit TOGGLE
 *   release (pin HIGH) — emit nothing
 *
 * Holding the button emits exactly once: the press edge is consumed and the state does not change
 * again until release, so there is no interval at which it could begin repeating.
 *
 * Debounce: 20 ms of post-fire suppression, not a stability window. An action's first edge *is* the
 * event, so waiting for the level to settle would only add latency. Suppressing after the send
 * costs nothing and stops contact chatter — which matters here more than for a level-tracking
 * switch, because every TOGGLE flips sim state: an even number of bounces cancels and an odd number
 * flips, so the final position would otherwise depend on bounce parity.
 *
 * forceReport() deliberately emits **nothing** — see its documentation.
 */
class ActionButton : public InputBase {
public:
    static constexpr uint32_t DEBOUNCE_MS = 20;

    /**
     * @brief Construct a momentary action button.
     *
     * @param controlId  DCSIN_* constant. Determines PanelBridge routing.
     * @param pin        PinRef for the button input pin (GPIO, MCP23017, or ShiftBus '165).
     * @param reverse    false (default): active-LOW — board wiring holds HIGH, button pulls LOW.
     *                   true: active-HIGH — board wiring holds LOW, button drives HIGH.
     *                   configure() does not enable internal pull-ups; the schematic must provide
     *                   the required pull-up, pull-down, or active drive.
     */
    ActionButton(uint16_t controlId, PinRef pin, bool reverse = false);

    /**
     * @brief Read the pin and emit one EVT_ACTION on a debounced press edge.
     *
     * Called by PanelGroup::loop() during normal operation. No-op until forceReport() has been
     * called at least once.
     */
    void poll() override;

    /**
     * @brief Establish the current pin state as the baseline. **Emits nothing.**
     *
     * Called by PanelGroup during the boot EVT burst and on every SYNC_REQ. Every other input
     * class emits its current position here, which is idempotent for a switch — re-sending
     * position 1 leaves the sim at 1. An action is not idempotent: emitting a TOGGLE would flip
     * the sim switch on every resync, so this method is silent by design.
     *
     * It is not a no-op, though. Seeding the baseline is what stops a button that is **physically
     * held at boot** from reading as a press edge on the first poll() and firing an unwanted
     * TOGGLE.
     */
    void forceReport() override;

    /**
     * @brief Configure the input pin. Called by PanelGroup::setup() after chip.begin().
     *
     * Does not enable internal pull-ups; board wiring supplies the input bias.
     *
     * @note Must not be called from the constructor — MCP23017 register writes require the chip to
     * be initialised first.
     */
    void configure() override;

    // sampleTick() is deliberately not overridden. RotaryEncoder needs it because quadrature
    // transitions can be lost between loop iterations; a button press lasts 50–100 ms, far longer
    // than any loop period, so the cached read is always current enough.

private:
    /** @brief The single polarity interpretation — used by both poll() and forceReport(). */
    bool readPressed() const;

    uint16_t _controlId;
    PinRef   _pin;
    bool     _reverse;        // true = active-HIGH (external pull-down required)
    bool     _lastPressed;    // last observed level, in press-polarity
    uint32_t _lastFireMs;     // millis() of the last emit — debounce reference
    bool     _initialized;    // false until forceReport() is called; poll() no-op before this
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
