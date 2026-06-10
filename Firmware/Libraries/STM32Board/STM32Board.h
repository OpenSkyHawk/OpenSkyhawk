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
 * @version 0.2.0
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

    // ── Deprecated — will be removed when CANProtocol is implemented ──────────
    // These remain temporarily so PanelGroup and PanelBridge compile unchanged
    // until the CANProtocol PR migrates those calls.

    /** @deprecated Use CANProtocol::start() instead. */
    void canStart();

    /** @deprecated Use CANProtocol::send() instead. */
    bool canSend(uint32_t canId, const uint8_t* data, uint8_t len);

    /** @deprecated Use CANProtocol::tec() instead. */
    uint8_t tec();

    /** @deprecated Use CANProtocol::rec() instead. */
    uint8_t rec();

    /** @deprecated Use CANProtocol::busOff() instead. */
    bool busOff();

} // namespace STM32Board

#endif // ARDUINO_ARCH_STM32
