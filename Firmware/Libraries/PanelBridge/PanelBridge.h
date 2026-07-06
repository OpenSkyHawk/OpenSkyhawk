/**
 * @file PanelBridge.h
 * @brief STM32 CAN master and DCS-BIOS processing node for OpenSkyhawk.
 *
 * PanelBridge bridges the RP2040 SimGateway (UART2, PA2/PA3) and the PanelGroup CAN
 * cluster. It runs the DCS-BIOS library on Serial, batches DCS output to CAN CTRL_BCAST
 * frames, routes CAN EVTs to DCS-BIOS commands or HID frames, and tracks PanelGroup
 * node liveness.
 *
 * Minimal production sketch:
 * @code
 * #define DCSBIOS_DEFAULT_SERIAL
 * #include <DcsBios.h>
 * #include <PanelBridge.h>
 *
 * void setup() {
 *     PanelBridge::setup();
 *     DcsBios::setup();
 * }
 * void loop() {
 *     DcsBios::loop();
 *     PanelBridge::loop();
 * }
 * @endcode
 *
 * @version 0.2.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <stdint.h>

namespace PanelBridge {

    /** @brief Callback type for node liveness events. Parameter is PanelGroup node ID (1–63). */
    using NodeCallback = void(*)(uint8_t nodeId);

    /**
     * @brief Register a callback fired when a node transitions from dead/unseen to alive.
     *
     * Passing nullptr clears the callback. Set before calling setup().
     *
     * @param cb Function called with the PanelGroup node ID (1–63).
     */
    void onNodeAlive(NodeCallback cb);

    /**
     * @brief Register a callback fired when a live node misses the heartbeat timeout (3 s).
     *
     * Passing nullptr clears the callback. Set before calling setup().
     *
     * @param cb Function called with the PanelGroup node ID (1–63).
     */
    void onNodeDead(NodeCallback cb);

    /**
     * @brief Initialise STM32 board services, UART2, CANProtocol, and PanelBridge internals.
     *
     * Performs in order: STM32Board::begin(), Serial.begin(250000), DCS-BIOS export listener
     * registration, CANProtocol::filterAcceptAll(), CANProtocol::onReceive() registration,
     * CANProtocol::start(), and cold-boot SYNC_REQ broadcast.
     *
     * @note Sketch must call DcsBios::setup() immediately after this.
     */
    void setup();

    /**
     * @brief Run PanelBridge work not owned by DcsBios::loop().
     *
     * Drains CANProtocol RX, dispatches EVT packets, services CANProtocol batching deadlines,
     * checks node heartbeat timeouts (3 s), and handles DiagSerial 'T' bytes for TEST_SEQ.
     *
     * @note Call after DcsBios::loop() each iteration.
     */
    void loop();

} // namespace PanelBridge

// ── Test hooks (PANELBRIDGE_TEST builds only) ─────────────────────────────────
// Analogous to SimGateway::feedByte(). Allow unit tests to exercise dispatch logic
// without a physical CAN bus or a live DCS-BIOS connection.
#ifdef PANELBRIDGE_TEST
namespace PanelBridge {

    /**
     * @brief Dispatch one EVT slot directly, bypassing CAN.
     *
     * Equivalent to receiving a CAN EVT frame with one non-null slot.
     * controlId < 0x8000 → 6-byte HID frame on Serial.
     * controlId 0x8000–0x86FF → DCS-BIOS ASCII command on Serial.
     * Other controlIds or 0x0000 (null sentinel) → dropped.
     *
     * @param controlId  Routing key (same semantics as CAN EVT slot).
     * @param value      Payload value.
     */
    void testDispatchEvt(uint16_t controlId, uint16_t value);

    /** @brief Test seam — dispatch one relative (canIdEvtRel) slot: value signed → `%+d` (variable_step). */
    void testDispatchRel(uint16_t controlId, uint16_t value);

    /** @brief Test seam — dispatch one directional (canIdEvtDir) slot: value ±1 → INC/DEC (fixed_step). */
    void testDispatchDir(uint16_t controlId, uint16_t value);

    /**
     * @brief Submit one DCS-BIOS export update directly, bypassing DcsBios::loop().
     *
     * Equivalent to the BridgeExportListener receiving address/value from DCS-BIOS.
     * Calls CANProtocol::sendBatched(CAN_ID_CTRL_BCAST, ...) with the given pair.
     *
     * @param address  DCS-BIOS export address (0x8000–0x86FF for cockpit outputs).
     * @param value    16-bit export value.
     */
    void testHandleExport(uint16_t address, uint16_t value);

#ifdef PANELBRIDGE_NODE_STATUS
    /**
     * @brief Cache a heartbeat and mark a node alive (bypassing CAN) (#86).
     *
     * Mirrors the HB branch of onCanRx: stores the payload as the node's last
     * heartbeat, then transitions it to alive — which emits one `_NODE_STATUS`
     * status message on Serial on the dead/unseen → alive edge.
     */
    void testFeedHeartbeat(uint8_t nodeId, uint8_t flags, uint16_t uptime,
                           uint16_t rxCount, uint16_t esr);

    /** @brief Cache a node's HEALTH_n telemetry (die temp) without changing liveness (#213). */
    void testFeedHealth(uint8_t nodeId, int8_t dieTempC);

    /** @brief Emit the current roster (one `_NODE_STATUS` per alive node) — the request/boot path. */
    void testRequestNodeStatus();

    /** @brief Run the timeout sweep with an injected clock; expires nodes and emits removals. */
    void testCheckTimeouts(uint32_t now);
#endif // PANELBRIDGE_NODE_STATUS

} // namespace PanelBridge
#endif // PANELBRIDGE_TEST

#endif // ARDUINO_ARCH_STM32
