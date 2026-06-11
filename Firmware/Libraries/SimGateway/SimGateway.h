/**
 * @file SimGateway.h
 * @brief RP2040 USB HID gateway library for OpenSkyhawk SimGateway board.
 *
 * Owns the USB CDC ↔ UART relay, 0xAA 0x55 HID frame demultiplexer, and
 * Joystick.Send() batching. HIDAxis and HIDButton objects are declared in the
 * sketch at file scope and self-register into linked lists at construction.
 * SimGateway::loop() walks those lists and dispatches matching HID frames.
 *
 * Does NOT run DCS-BIOS, parse DCS-BIOS addresses, or interact with CAN.
 * Platform: RP2040 only.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_RP2040

#include <Arduino.h>
#include <HIDControls.h>

namespace OpenSkyhawk {

/**
 * @brief HID axis handler. Declared at sketch scope for each joystick axis.
 *
 * Self-registers into a static linked list at construction. SimGateway::loop()
 * walks the list and calls Joystick.SetAxis() when a HID frame with a matching
 * controlId arrives. The 0–65535 unsigned value from PanelGroup is mapped to
 * ±32767 internally (value − 32768). The sketch has no knowledge of this mapping.
 *
 * Declare at file scope, not inside functions — C++ constructs file-scope objects
 * before setup() runs, which is required for the linked list to be populated.
 */
class HIDAxis {
public:
    /**
     * @brief Register a HID axis handler.
     * @param controlId  CTRL_* constant from HIDControls.h (0x0010–0x001F range).
     * @param axisIndex  MGS-Pico-Joystick axis index (0–7).
     */
    HIDAxis(uint16_t controlId, uint8_t axisIndex);

    static HIDAxis* head();        ///< First registered axis; nullptr if none.
    uint16_t controlId() const;    ///< controlId this handler is registered for.
    /**
     * @brief Dispatch a value to the Joystick axis.
     * @param value  Unsigned 0–65535; mapped to int16_t (value − 32768) internally.
     */
    void     dispatch(uint16_t value);
    HIDAxis* next() const;         ///< Next axis in list; nullptr at end.

private:
    static HIDAxis* _head;
    HIDAxis*        _next;
    uint16_t        _controlId;
    uint8_t         _axisIndex;
};

/**
 * @brief HID button handler. Declared at sketch scope for each button.
 *
 * Self-registers into a static linked list at construction.
 * value != 0 → button pressed; value == 0 → button released.
 */
class HIDButton {
public:
    /**
     * @brief Register a HID button handler.
     * @param controlId   CTRL_* constant from HIDControls.h (0x0020–0x009F range).
     * @param buttonIndex Joystick button index (0–127).
     */
    HIDButton(uint16_t controlId, uint8_t buttonIndex);

    static HIDButton* head();
    uint16_t controlId() const;
    /**
     * @brief Dispatch a value to the Joystick button.
     * @param value  0 → released; non-zero → pressed.
     */
    void      dispatch(uint16_t value);
    HIDButton* next() const;

private:
    static HIDButton* _head;
    HIDButton*        _next;
    uint16_t          _controlId;
    uint8_t           _buttonIndex;
};

} // namespace OpenSkyhawk

namespace SimGateway {

/**
 * @brief Initialise USB identity, Joystick, and UART link to PanelBridge.
 *
 * Must be the first call in the sketch's setup(). Sets USB identity before the
 * TinyUSB stack enumerates:
 *   - Manufacturer: "OpenSkyhawk"
 *   - Product:      "A-4E Skyhawk"
 *   - VID/PID:      0x2E8A / 0x4134
 * Calls uart.begin(250000). Configures MGS-Pico-Joystick for 16-bit axes and
 * manual send mode.
 *
 * @param uart  Hardware UART connected to PanelBridge (Serial1, GP0/GP1 on standard board).
 */
void setup(HardwareSerial& uart);

/**
 * @brief Relay bytes and dispatch HID frames. Call once per loop() iteration.
 *
 * Per call:
 *   1. Forward all USB CDC bytes (Serial) to UART — DCS-BIOS stream to PanelBridge.
 *   2. Drain all UART bytes:
 *        byte ≤ 0x7F             → forward to USB CDC (DCS-BIOS from PanelBridge)
 *        0xAA 0x55 + 4 bytes     → parse controlId + value LE; dispatch to HIDAxis/HIDButton lists
 *        0xAA + non-0x55         → forward 0xAA + byte to USB CDC; resume IDLE
 *   3. If any HID setter fired, call Joystick.Send() exactly once.
 *
 * @note Parser state persists across calls — frames split across iterations assemble correctly.
 */
void loop();

#ifdef SIMGATEWAY_TEST
/**
 * @brief Feed one byte into the parser (test builds only).
 * @return true if a HID setter fired for this byte.
 * @note Define SIMGATEWAY_TEST to enable. Not available in production builds.
 */
bool feedByte(uint8_t b);

/** @brief Reset parser to IDLE state (test builds only). Call between test cases. */
void resetParser();
#endif

} // namespace SimGateway

#endif // ARDUINO_ARCH_RP2040
