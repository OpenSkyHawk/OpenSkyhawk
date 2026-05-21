/**
 * @file STM32Board.h
 * @brief Shared STM32F103 hardware initialisation for OpenSkyhawk avionics nodes.
 *
 * @details Provides a single definition of the CAN peripheral MSP init, 500 kbps
 * CAN setup, diagnostic UART, and status LED — all of which are identical on every
 * OpenSkyhawk STM32F103CBT6 board. PanelGroup and PanelBridge call these functions
 * instead of duplicating them.
 *
 * Fixed hardware pinouts (same on every board — no constructor arguments):
 *   - CAN  : SN65HVD230 on PA11 (RX) / PA12 (TX)
 *   - UART : USART1 PA9 TX / PA10 RX @ 115200 (diagnostic tap, TX-only in practice)
 *   - LED  : PC13, active LOW
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <stm32f1xx_hal_can.h>

/**
 * @brief Shared hardware layer for all OpenSkyhawk STM32F103 avionics nodes.
 *
 * @details Static singleton — call begin() once in setup(), update() every loop
 * iteration. All functions are free functions; no instantiation required.
 */
namespace STM32Board {

    /**
     * @brief Initialise LED, DiagSerial, and CAN HAL peripheral.
     *
     * @details Does NOT call HAL_CAN_Start. The caller (PanelGroup or PanelBridge)
     * must configure the RX filter for its use case, then call canStart().
     */
    void begin();

    /**
     * @brief Start the CAN peripheral. Call after configuring the RX filter.
     */
    void canStart();

    /**
     * @brief Blink the status LED based on CAN bus health. Call every loop iteration.
     *
     * @details Blink rate: 500 ms normal, 100 ms when TEC > 0, solid ON when bus-off.
     */
    void update();

    /**
     * @brief Enable or disable diagnostic serial output.
     *
     * @details DiagSerial is always initialised in begin() — this flag gates all
     * log() calls and isDebug() checks so no CPU cycles are spent on string
     * formatting or UART transmission when debug is off (the default).
     *
     * @param on true to enable output, false to suppress it.
     */
    void setDebug(bool on);

    /**
     * @brief Print a message to DiagSerial if debug is enabled.
     * @param msg Null-terminated string to print.
     */
    void log(const char* msg);

    /**
     * @brief Send a standard-frame CAN message.
     *
     * @param canId  11-bit standard CAN identifier.
     * @param data   Pointer to payload bytes.
     * @param len    Payload length in bytes (0–8).
     * @returns true on success, false if all TX mailboxes are full.
     */
    bool canSend(uint32_t canId, const uint8_t* data, uint8_t len);

    /**
     * @brief Return the CAN Transmit Error Counter from the ESR register.
     * @returns TEC value (0–255).
     */
    uint8_t tec();

    /**
     * @brief Return the CAN Receive Error Counter from the ESR register.
     * @returns REC value (0–255).
     */
    uint8_t rec();

    /**
     * @brief Return whether the CAN controller is in bus-off state.
     * @returns true if ESR BOFF bit is set.
     */
    bool busOff();

    /**
     * @brief Returns true when debug output is enabled.
     *
     * @details Use to guard multi-field formatted print blocks in PanelGroup
     * and PanelBridge so they are skipped entirely when debug is off.
     *
     * @returns true if setDebug(true) has been called.
     */
    bool isDebug();

    /**
     * @brief Access the HAL CAN handle for RX filter configuration.
     *
     * @details Called by PanelGroup::setup() and PanelBridge::setup() to configure
     * their respective CAN receive filters before calling canStart().
     *
     * @returns Pointer to the internal CAN_HandleTypeDef.
     */
    CAN_HandleTypeDef* canHandle();

    /**
     * @brief Access DiagSerial for multi-field formatted output.
     *
     * @details Guard calls with isDebug() to avoid string formatting overhead
     * when debug is off.
     *
     * @returns Reference to the USART1 HardwareSerial instance.
     */
    HardwareSerial& diagSerial();

} // namespace STM32Board

#endif // ARDUINO_ARCH_STM32
