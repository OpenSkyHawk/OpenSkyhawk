/**
 * @file SimGateway.h
 * @brief RP2040 USB HID + DCS-BIOS gateway domain layer for OpenSkyhawk.
 *
 * @details Owns USB device identity, Joystick HID composite device, and the UART
 * link to the PanelBridge STM32. Parses incoming DIAG frames from PanelBridge and
 * dispatches them to registered callbacks. DCS-BIOS setup/loop must be called by
 * the sketch — they require `#define DCSBIOS_DEFAULT_SERIAL` in the sketch's
 * translation unit before `#include <DcsBios.h>`.
 *
 * A minimal gateway sketch:
 *
 * @code
 * #define DCSBIOS_DEFAULT_SERIAL
 * #include <DcsBios.h>
 * #include <SimGateway.h>
 *
 * DcsBios::IntegerBuffer rpmBuf(A_4E_C_RPM, onRpmChange);
 *
 * void setup() {
 *     SimGateway::setup(Serial1);
 *     DcsBios::setup();
 * }
 * void loop() {
 *     DcsBios::loop();
 *     SimGateway::loop();
 * }
 * @endcode
 *
 * @note Including SimGateway.h pulls in Joystick.h transitively, so sketches can
 *       call Joystick.button() etc. without an extra include.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_RP2040

#include <Arduino.h>
#include <Joystick.h>   ///< Exposed so sketches can call Joystick.button() etc.
#include <CANProtocol.h>

/**
 * @brief Static singleton for the RP2040 DCS-BIOS / USB HID gateway.
 *
 * @details Sets the USB VID/PID and product strings, initialises the Joystick HID
 * composite device, and starts the UART link to PanelBridge at 250000 baud.
 * In loop(), drains the UART and dispatches 8-byte DIAG frames to registered
 * callbacks. Sending ControlPackets to PanelBridge is done via send().
 */
namespace SimGateway {

    /**
     * @brief Initialise USB identity, Joystick, and UART link to PanelBridge.
     *
     * @details Must be called before DcsBios::setup() so the USB identity is set
     * before the composite device stack starts. Sets:
     *   - Manufacturer: "OpenSkyhawk", Product: "A-4E Skyhawk"
     *   - VID/PID: 0x2E8A / 0x4134
     *
     * @param panelBridgePort UART port connected to the PanelBridge STM32.
     *                        Use Serial1 (GP0 TX / GP1 RX) on the standard gateway board.
     */
    void setup(HardwareSerial& panelBridgePort);

    /**
     * @brief Drain the UART from PanelBridge and dispatch DIAG frames.
     *
     * @details Reads incoming bytes from panelBridgePort, re-syncs on DIAG_MAGIC,
     * and calls the registered onDiagRtt / onDiagHb / onDiagErr callbacks when
     * a complete 8-byte DIAG frame is assembled. Call DcsBios::loop() in your
     * sketch's loop() before or after this.
     */
    void loop();

    /**
     * @brief Send a ControlPacket to PanelBridge over UART.
     *
     * @details Packs controlId and value into a 4-byte ControlPacket struct and
     * writes it to panelBridgePort. PanelBridge will broadcast it on CAN as a
     * CTRL_BCAST frame (or as a TEST_SEQ frame if controlId == CTRL_TEST_SEQ).
     *
     * @param controlId Identifies the target control (HID, DCS-BIOS address, or CTRL_TEST_SEQ).
     * @param value     Value to send.
     */
    void send(uint16_t controlId, uint16_t value);

    /**
     * @brief Register a callback for DIAG_RTT frames from PanelBridge.
     *
     * @details Called when PanelBridge forwards a sub-node echo back as a round-trip
     * time measurement frame. seq matches the sequence number sent with CTRL_TEST_SEQ;
     * sentMs is the PanelBridge-side timestamp when the TEST_SEQ CAN frame was sent
     * (use millis() - pingSentMs on the RP2040 side to compute RTT).
     *
     * @note Set before calling setup().
     * @param cb Callback: cb(seq, sentMs).
     */
    void onDiagRtt(void (*cb)(uint16_t seq, uint32_t sentMs));

    /**
     * @brief Register a callback for DIAG_HB (sub-node heartbeat) frames.
     *
     * @note Set before calling setup().
     * @param cb Callback: cb(nodeId, rxCount).
     */
    void onDiagHb(void (*cb)(uint8_t nodeId, uint16_t rxCount));

    /**
     * @brief Register a callback for DIAG_ERR (CAN error counter) frames.
     *
     * @note Set before calling setup().
     * @param cb Callback: cb(tec, rec, flags). flags bits: 0x01=bus-off, 0x02=error-passive.
     */
    void onDiagErr(void (*cb)(uint8_t tec, uint8_t rec, uint8_t flags));

    /**
     * @brief Register a callback for DIAG_EVT (sub-node input event) frames.
     *
     * @details Called when PanelBridge forwards a CAN input event from a sub-node.
     * controlId and value are the raw PanelGroup::sendEvent() arguments; nodeId
     * identifies which sub-node sent the event (1 or 2). Use this to translate
     * the event to a DCS-BIOS message via sendDcsBiosMessage() in the sketch.
     *
     * @note Set before calling setup().
     * @param cb Callback: cb(controlId, value, nodeId).
     */
    void onDiagEvt(void (*cb)(uint16_t controlId, uint16_t value, uint8_t nodeId));

} // namespace SimGateway

#endif // ARDUINO_ARCH_RP2040
