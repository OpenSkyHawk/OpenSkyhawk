/**
 * @file STM32Board.h
 * @brief Shared STM32F103 hardware initialisation for OpenSkyhawk avionics nodes.
 *
 * @details Manages the bi-color status LED, DiagSerial, and CAN peripheral
 * configuration — identical on every OpenSkyhawk STM32F103CBT6 board.
 * All CAN bus operations (send, filter, start) go through CANProtocol.
 *
 * Fixed hardware (same on every board — no constructor arguments):
 *   - LED  : PB14 (red) + PB15 (green), active HIGH
 *   - UART : USART1 PA9 TX / PA10 RX @ 115200 (diagnostic tap)
 *   - CAN  : SN65HVD230 on PA11 (RX) / PA12 (TX)
 *
 * @version 0.4.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <stm32f1xx_hal_can.h>

// CanStatus is owned by CANProtocol; forward-declared here so onCanStatus()
// can accept it without pulling the full CANProtocol header into every user.
enum class CanStatus;

static_assert(NODE_ID <= 63,
    "NODE_ID must be 0-63. 0 is reserved for PanelBridge; 1-63 for PanelGroup nodes.");

#ifdef STM32BOARD_TEST
// Effective status-LED state. Normally an internal type defined in STM32Board.cpp;
// exposed here (and mirrored in the .cpp) only for on-target test assertions —
// see Firmware/Tests/STM32Board/. Keep both definitions in sync.
enum class LedState {
    OFF,       ///< Both LEDs off — pre-begin() only
    BOOTING,   ///< Red slow blink (1000 ms) — initialising
    NORMAL,    ///< Green slow blink (1000 ms) — CAN healthy, no data flowing
    CONNECTED, ///< Green solid — CAN healthy and data flowing (DCS exports / CTRL_BCAST)
    CAN_ERROR, ///< Red fast blink (250 ms) — TEC > 0, errors accumulating
    BUS_OFF,   ///< Red solid — CAN controller halted
    WARNING,   ///< Red/green alternating (500 ms) — app-layer degraded state
};
#endif

namespace STM32Board {

    static constexpr uint8_t PIN_LED_RED   = PB14; ///< Red LED pin — same on all STM32 boards
    static constexpr uint8_t PIN_LED_GREEN = PB15; ///< Green LED pin — same on all STM32 boards

    /**
     * @brief Initialise all shared hardware. Call once at the top of setup().
     *
     * Configures PB14 (Red) and PB15 (Green) as outputs, enters BOOTING LED state.
     * Starts DiagSerial (USART1 PA9/PA10, 115200 baud) — silent until setDebug(true).
     * Configures the CAN peripheral at 500 kbps on PA11/PA12 but does NOT start it —
     * call CANProtocol::start() after filter setup.
     */
    void begin();

    /**
     * @brief Enable or disable DiagSerial output.
     *
     * DiagSerial is always initialised by begin(); this flag gates all log() calls.
     *
     * @param on True to emit output on USART1; false for silence (default).
     */
    void setDebug(bool on);

    /**
     * @brief Drive LED animations. Call once per loop() iteration.
     *
     * Advances blink state using millis(). Fully non-blocking.
     */
    void tick();

    /**
     * @brief CAN bus status event handler — maps CanStatus to LED state.
     *
     * Register with CANProtocol::onStatusChange(STM32Board::onCanStatus) in setup().
     * Never call directly from sketch code.
     *
     * @param status New CAN bus status reported by CANProtocol.
     */
    void onCanStatus(CanStatus status);

    /**
     * @brief Raise or clear the WARNING condition — red/green alternating at 500 ms.
     *
     * Call when a degraded condition is detected that is not represented by CanStatus
     * (e.g. SYNC timeout, dead PanelGroup node, lost master heartbeat). WARNING outranks
     * CONNECTED/NORMAL but is masked by any CAN fault (CAN_ERROR/BUS_OFF). Clear it with
     * setWarning(false) once the condition recovers.
     *
     * @param on True to raise WARNING (default); false to clear it.
     */
    void setWarning(bool on = true);

    /**
     * @brief Signal that application data is actively flowing → CONNECTED (green solid).
     *
     * Call setLinkActive(true) on each unit of inbound data (PanelBridge: a DCS-BIOS
     * export seen; PanelGroup: a CTRL_BCAST received). The link auto-decays back to
     * NORMAL after ~500 ms with no further calls. CONNECTED is only shown while the CAN
     * bus is healthy; a CAN fault masks it and it re-engages automatically on recovery
     * if data is still flowing.
     *
     * @param active True to (re)assert the data-flowing link; false to drop it immediately.
     */
    void setLinkActive(bool active);

    /**
     * @brief Returns true when debug output is enabled.
     *
     * Guard multi-field formatted print blocks with this to skip string formatting
     * overhead when debug is off.
     */
    bool isDebug();

    /**
     * @brief Print a line to DiagSerial if debug is enabled; no-op otherwise.
     * @param msg Null-terminated string to print.
     */
    void log(const char* msg);

    /**
     * @brief Access DiagSerial directly for multi-field formatted output.
     *
     * Guard with isDebug() to avoid formatting overhead when debug is off.
     *
     * @returns Reference to the USART1 HardwareSerial instance.
     */
    HardwareSerial& diagSerial();

    /**
     * @brief Access the HAL CAN handle.
     *
     * @note Internal — used by CANProtocol only. Do not call from sketches.
     * @returns Pointer to the internal CAN_HandleTypeDef.
     */
    CAN_HandleTypeDef* canHandle();

    /**
     * @brief Read the MCU's built-in internal temperature sensor (ADC ch16).
     *
     * Free per-node thermal telemetry — no external parts, no PCB change. Reads ATEMP
     * and AVREF (Vrefint) and converts with STM32F103 datasheet typicals
     * (V25 = 1.43 V, Avg_Slope = 4.3 mV/°C), referencing Vsense to the measured Vdd.
     *
     * @note UNCALIBRATED: no factory trim → ~±few °C absolute; measures DIE temperature
     *       (not ambient) with a self-heat offset. Use for relative trend / overheat
     *       flagging, not precise ambient measurement.
     * @return Die temperature in whole °C, or INT8_MIN if the internal channels are
     *         unavailable on this variant.
     */
    int8_t readDieTempC();

    /**
     * @brief Estimate MCU Vdd from the internal reference (Vrefint, ADC ch17).
     *
     * @note Uses the STM32F103 typical Vrefint of 1.20 V (the F103 has no VREFINT_CAL
     *       factory value), so absolute accuracy is limited; good for relative trend.
     * @return Vdd in millivolts, or 0 if the internal reference is unavailable.
     */
    uint16_t readVddMv();

#ifdef STM32BOARD_TEST
    /**
     * @brief Test-only: the current effective status-LED state.
     *
     * Gated by STM32BOARD_TEST so production builds keep LedState internal. Used by the
     * on-target test sketches under Firmware/Tests/STM32Board/ to assert state transitions,
     * precedence, and link decay without racing the blink animator.
     */
    LedState currentState();
#endif

} // namespace STM32Board

#endif // ARDUINO_ARCH_STM32
