/**
 * @file PanelBridge.h
 * @brief CAN master / UART bridge domain layer for OpenSkyhawk.
 *
 * @details Provides a transparent bridge between the RP2040 SimGateway (over UART)
 * and the CAN sub-nodes (PanelGroup boards). ControlPackets received from the RP2040
 * are broadcast on the CAN bus; frames received from sub-nodes are forwarded to the
 * RP2040 as DIAG frames. Sub-node heartbeat watchdog fires callbacks on liveness changes.
 *
 * A minimal master sketch:
 *
 * @code
 * #include <PanelBridge.h>
 *
 * void setup() {
 *     STM32Board::setDebug(true);
 *     PanelBridge::setup(Serial2);   // UART2 PA2/PA3 @ 250000
 * }
 * void loop() { PanelBridge::loop(); }
 * @endcode
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <CANProtocol.h>

/**
 * @brief Static singleton for CAN master / UART bridge firmware.
 *
 * @details Bridges ControlPacket structs between the RP2040 SimGateway (UART) and the
 * CAN bus. Accepts all CAN frames. Forwards heartbeats and RTT echo frames to the
 * RP2040 as 8-byte DIAG frames (DIAG_MAGIC framing defined in CANProtocol.h).
 * Fires optional callbacks when a sub-node comes alive or goes silent.
 */
namespace PanelBridge {

    /**
     * @brief Initialise hardware and start the CAN bus.
     *
     * @details Calls STM32Board::begin(), starts the UART at 250000 baud,
     * configures a pass-all CAN filter, then calls canStart().
     * Call STM32Board::setDebug(true) before this to enable diagnostic output.
     *
     * @param uartPort Hardware serial port connected to the RP2040 SimGateway.
     *                 Use Serial2 (UART2 PA2/PA3) on the standard master board.
     */
    void setup(HardwareSerial& uartPort);

    /**
     * @brief Service all PanelBridge activity. Call every Arduino loop() iteration.
     *
     * @details Calls STM32Board::update(), drains the UART RX buffer and processes
     * incoming ControlPackets, drains the CAN RX FIFO and forwards frames to the
     * RP2040, and checks the sub-node heartbeat watchdog.
     */
    void loop();

    /**
     * @brief Register a callback invoked when a sub-node sends its first heartbeat
     *        after a dead or startup period.
     *
     * @note Set before calling setup().
     * @param cb Function called with the node_id of the newly live node.
     */
    void onNodeAlive(void (*cb)(uint8_t nodeId));

    /**
     * @brief Register a callback invoked when no heartbeat is received from any
     *        sub-node for 3 seconds.
     *
     * @note Set before calling setup().
     * @param cb Function called with the node_id of the timed-out node.
     *           In the current prototype, node_id is always 1; multi-node
     *           tracking is a future enhancement.
     */
    void onNodeDead(void (*cb)(uint8_t nodeId));

} // namespace PanelBridge

#endif // ARDUINO_ARCH_STM32
