/**
 * @file Switch2Pos.h
 * @brief Debounced 2-position switch for OpenSkyhawk PanelGroup nodes.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // InputBase, PinRef

namespace OpenSkyhawk {

/**
 * @brief Debounced 2-position switch. Self-registers into PanelGroup's InputBase list.
 *
 * @details VALUE semantics (reverse = false, default):
 *   1 — active   (pin LOW — switch closed, pulling pin to GND via board pull-up)
 *   0 — inactive (pin HIGH)
 *
 * VALUE semantics (reverse = true):
 *   1 — active   (pin HIGH — switch drives pin HIGH; external pull-down holds pin LOW when open)
 *   0 — inactive (pin LOW)
 *
 * Debounce: fixed 20 ms. The pin must hold its new level for the debounce period before
 * the state is confirmed and a CAN EVT is emitted. Any level change during the window
 * restarts the timer.
 *
 * forceReport() emits the current physical state immediately without debounce — called
 * by PanelGroup during the boot EVT burst and on SYNC_REQ.
 */
class Switch2Pos : public InputBase {
public:
    static constexpr uint16_t DEBOUNCE_MS = 20;

    /**
     * @brief Construct a 2-position switch with default settings.
     *
     * Active-LOW (reverse = false), 20 ms debounce.
     *
     * @param controlId  DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pin        PinRef for the switch input pin (GPIO or MCP23017).
     */
    Switch2Pos(uint16_t controlId, PinRef pin);

    /**
     * @brief Construct a 2-position switch with explicit polarity.
     *
     * @param controlId   DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pin         PinRef for the switch input pin (GPIO or MCP23017).
     * @param reverse     false (default): active-LOW — board wiring holds HIGH, switch pulls LOW.
     *                    true: active-HIGH — board wiring holds LOW, switch drives HIGH.
     *                    configure() does not enable internal pull-ups; the schematic must
     *                    provide the required pull-up, pull-down, or active drive.
     */
    Switch2Pos(uint16_t controlId, PinRef pin, bool reverse);

    /**
     * @brief Read current pin state, apply debounce, emit EVT if confirmed state changed.
     *
     * Called by PanelGroup::loop() during normal operation. No-op until forceReport()
     * has been called at least once.
     */
    void poll() override;

    /**
     * @brief Read current pin state and emit EVT unconditionally — no debounce.
     *
     * Called by PanelGroup during the boot EVT burst and on SYNC_REQ. Confirms the
     * current reading as the baseline so subsequent poll() calls have a valid reference.
     */
    void forceReport() override;

    /**
     * @brief Configure the input pin. Called by PanelGroup::setup() after chip.begin().
     *
     * Configures the pin as an input. Does not enable internal pull-ups; board wiring
     * supplies the input bias. Typical OpenSkyhawk switch nets use external 10 kΩ
     * pull-ups to +3.3V and switch to GND.
     *
     * @note Must not be called from the constructor — MCP23017 register writes require
     * the chip to be initialised first.
     */
    void configure() override;

private:
    uint16_t _controlId;
    PinRef   _pin;
    bool     _reverse;          // true = active-HIGH (external pull-down required)
    bool     _lastConfirmed;    // last emitted state (true = active)
    bool     _pendingRaw;       // raw reading at the last level change
    uint32_t _debounceStartMs;  // millis() when _pendingRaw last changed
    bool     _initialized;      // false until forceReport() is called; poll() no-op before this
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
