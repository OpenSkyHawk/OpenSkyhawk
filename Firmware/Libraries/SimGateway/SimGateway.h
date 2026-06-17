/**
 * @file SimGateway.h
 * @brief RP2040 USB HID gateway library for OpenSkyhawk SimGateway board.
 *
 * Owns the USB CDC ↔ UART relay, 0xAA 0x55 HID frame demultiplexer, and
 * OsJoystick.send() batching. HIDAxis, HIDButton, and HIDHatSwitch objects are
 * declared in the sketch at file scope and self-register into linked lists at
 * construction. SimGateway::loop() walks those lists and dispatches matching HID
 * frames to the OpenSkyhawkJoystick abstraction layer.
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
 * walks the list and calls OsJoystick.setAxis() when a HID frame with a matching
 * controlId arrives. The 0–65535 unsigned value from PanelGroup is mapped to
 * signed ±32767 internally (value − 32768).
 *
 * Declare at file scope, not inside functions — C++ constructs file-scope objects
 * before setup() runs, which is required for the linked list to be populated.
 */
class HIDAxis {
public:
    /**
     * @brief Register a HID axis handler.
     * @param controlId  CTRL_* constant from HIDControls.h (0x0010–0x001F range).
     * @param axisIndex  OpenSkyhawkJoystick axis index (0–7).
     */
    HIDAxis(uint16_t controlId, uint8_t axisIndex);

    static HIDAxis* head();       ///< First registered axis; nullptr if none.
    uint16_t controlId() const;   ///< controlId this handler is registered for.
    /**
     * @brief Dispatch a value to the joystick axis.
     * @param value  Unsigned 0–65535; mapped to int16_t (value − 32768) internally.
     */
    void     dispatch(uint16_t value);
    HIDAxis* next() const;        ///< Next axis in list; nullptr at end.

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
     * @param controlId   CTRL_* button constant from HIDControls.h (0x0030–0x00AF range).
     * @param buttonIndex OpenSkyhawkJoystick button index (0–127).
     */
    HIDButton(uint16_t controlId, uint8_t buttonIndex);

    static HIDButton* head();
    uint16_t controlId() const;
    /**
     * @brief Dispatch a value to the joystick button.
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

/**
 * @brief HID hat switch handler. Declared at sketch scope for each hat switch.
 *
 * Self-registers into a static linked list at construction.
 * Dispatches to OsJoystick.setHat() with direction clamped to 0–8.
 */
class HIDHatSwitch {
public:
    /**
     * @brief Register a HID hat switch handler.
     * @param controlId  CTRL_* constant from HIDControls.h (0x0020–0x002F range).
     * @param hatIndex   OpenSkyhawkJoystick hat index (0–3).
     */
    HIDHatSwitch(uint16_t controlId, uint8_t hatIndex);

    static HIDHatSwitch* head();
    uint16_t controlId() const;
    /**
     * @brief Dispatch a direction to the hat switch.
     * @param value  0 = centered, 1 = N, 2 = NE, 3 = E, 4 = SE,
     *               5 = S, 6 = SW, 7 = W, 8 = NW. Values > 8 → centered.
     */
    void          dispatch(uint16_t value);
    HIDHatSwitch* next() const;

private:
    static HIDHatSwitch* _head;
    HIDHatSwitch*        _next;
    uint16_t             _controlId;
    uint8_t              _hatIndex;
};

} // namespace OpenSkyhawk

namespace SimGateway {

static constexpr uint8_t DEFAULT_UART_TX_PIN = 0; ///< RP2040 UART0 TX to PanelBridge RX.
static constexpr uint8_t DEFAULT_UART_RX_PIN = 1; ///< RP2040 UART0 RX from PanelBridge TX.

/**
 * @brief Initialise USB identity, OpenSkyhawkJoystick, and UART link to PanelBridge.
 *
 * Must be the first call in the sketch's setup(). Sets USB identity before the
 * TinyUSB stack enumerates:
 *   - Manufacturer: "OpenSkyhawk"
 *   - Product:      "A-4E Skyhawk"
 *   - VID/PID:      0x2E8A / 0x4134
 *   - CDC port:     "A-4E Skyhawk DCS-BIOS" (iInterface — names the serial port)
 * Configures the UART pins and calls uart.begin(250000), then calls
 * OsJoystick.begin() to initialise the HID descriptor and enumerate.
 *
 * @param uart   Hardware UART connected to PanelBridge (Serial1 / UART0 on standard board).
 * @param txPin  RP2040 UART TX pin. Defaults to GP0, wired to STM32 PA3.
 * @param rxPin  RP2040 UART RX pin. Defaults to GP1, wired to STM32 PA2.
 */
void setup(SerialUART& uart,
           uint8_t txPin = DEFAULT_UART_TX_PIN,
           uint8_t rxPin = DEFAULT_UART_RX_PIN);

/**
 * @brief Relay bytes and dispatch HID frames. Call once per loop() iteration.
 *
 * Per call:
 *   1. Forward all USB CDC bytes (Serial) to UART — DCS-BIOS stream to PanelBridge.
 *   2. Drain all UART bytes:
 *        byte ≤ 0x7F             → forward to USB CDC (DCS-BIOS from PanelBridge)
 *        0xAA 0x55 + 4 bytes     → parse controlId + value LE; dispatch to HID lists
 *        0xAA + non-0x55         → forward 0xAA + byte to USB CDC; resume IDLE
 *   3. If any HID setter fired, call OsJoystick.send() exactly once.
 *
 * @note Node-status reporting (#86) needs no handling here: PanelBridge's `_NODE_STATUS` DCS-BIOS
 *       messages are ASCII (≤ 0x7F) forwarded by step 2, and the host's roster request is a
 *       DCS-BIOS export write (addr 0x86FE) forwarded by step 1. Both transit transparently.
 *
 * @note Parser state persists across calls — frames split across iterations assemble correctly.
 */
void loop();

// ── Status LEDs (Gateway_Bridge board) ────────────────────────────────────────
//
// Two board-mounted status LEDs report the gateway's USB / data / fault state at a
// glance: RED = GP3, GREEN = GP2, board-mounted 0805, active-high (HIGH = on). The
// animator is non-blocking (millis(), never delay()) and is ticked from loop(), so
// sketches need no LED code. The RP2040 module's onboard WS2812 is not used.
// See docs/architecture/sim-gateway.md for the user-facing table.

/**
 * @brief SimGateway status states, highest priority first.
 *
 * Resolved each tick; the highest-priority active state wins.
 */
enum class LedState : uint8_t {
    FAULT,     ///< uart0 hardware error (RSR) — RED FAST. Latched until clean data resumes (≥2 s hold).
    NO_HOST,   ///< USB not enumerated, or unplugged after a mount — RED SOLID.
    STREAMING, ///< DCS-BIOS bytes from the PC within the last 500 ms — GREEN SOLID.
    USB_IDLE,  ///< USB mounted, no recent DCS data — GREEN SLOW (1 Hz).
    INIT,      ///< Booted, never mounted, within the init window — RED SLOW (brief).
};

/** @brief Non-blocking animation patterns (millis()-based). */
enum class Anim : uint8_t {
    OFF,   ///< Both off.
    SOLID, ///< Steady on.
    SLOW,  ///< 1 Hz blink (1000 ms period).
    FAST,  ///< 4 Hz blink (250 ms period).
    ALT,   ///< Red/green alternate (500 ms) — reserved; no state uses it yet.
    PULSE, ///< Brief ~50 ms off-blip on traffic — disabled by default (STREAMING is SOLID).
};

/**
 * @brief Configure GP2 (green) / GP3 (red) as outputs, both off. Call once from setup().
 *
 * Called automatically by SimGateway::setup(); sketches do not call it directly.
 */
void statusLedBegin();

/**
 * @brief Advance the status-LED state machine and animation. Call once per loop().
 *
 * Samples USB-mount, recent DCS-BIOS activity, and the uart0 PL011 error flags, then
 * drives GP2/GP3 for the current state. Non-blocking. Called automatically by
 * SimGateway::loop(); sketches do not call it directly.
 */
void statusTick();

#ifdef SIMGATEWAY_TEST
/**
 * @brief Feed one byte into the parser (test builds only).
 * @return true if a HID setter fired for this byte.
 * @note Define SIMGATEWAY_TEST to enable. Not available in production builds.
 */
bool feedByte(uint8_t b);

/** @brief Reset parser to IDLE state (test builds only). Call between test cases. */
void resetParser();

/** @brief Clear captured parser-forwarded USB CDC bytes (test builds only). */
void resetCdcCapture();

/** @brief Number of parser-forwarded USB CDC bytes captured since resetCdcCapture(). */
size_t cdcCaptureCount();

/** @brief Captured parser-forwarded USB CDC byte at index, or 0 if out of range. */
uint8_t cdcCaptureByte(size_t index);

/** @brief True if more CDC bytes were forwarded than the test capture buffer can hold. */
bool cdcCaptureOverflow();

// ── Status-LED test hooks (test builds only) ──────────────────────────────────
// The pure state-selection + animation logic is exercised without GPIO, TinyUSB,
// or PL011 register reads by injecting inputs and reading back the resolved state /
// captured pin levels. Mirrors the feedByte/cdcCapture pattern above.

/**
 * @brief Inject fully-resolved status inputs (test builds). Pairs with statusResolve().
 * @param now          Fake millis() timestamp.
 * @param mounted      Fake USB-mounted flag (sticks _everMounted once true).
 * @param lastCdcRxMs  Timestamp of the last CDC→UART activity.
 * @param faultActive  Resolved FAULT-latch flag (use statusFaultStep() to derive it).
 */
void statusInject(uint32_t now, bool mounted, uint32_t lastCdcRxMs, bool faultActive);

/** @brief Resolve + apply the injected inputs; updates statusState/Anim/Red/Green. */
void statusResolve();

/**
 * @brief Drive the FAULT latch one tick (test builds). Returns the resolved faultActive.
 * @param now          Fake millis() timestamp.
 * @param rsrError     A PL011 error bit was set this tick (overrun/framing/parity/break).
 * @param uartRxMoved  ≥1 error-free byte was read from the UART this tick.
 */
bool statusFaultStep(uint32_t now, bool rsrError, bool uartRxMoved);

/** @brief Last resolved LedState from statusResolve() (test builds). */
LedState statusState();

/** @brief Last resolved Anim from statusResolve() (test builds). */
Anim statusAnim();

/** @brief Captured GP3 (red) level from the last statusResolve() (test builds). */
bool statusRedLevel();

/** @brief Captured GP2 (green) level from the last statusResolve() (test builds). */
bool statusGreenLevel();

/** @brief Reset all status-LED latches/timestamps between test cases (test builds). */
void statusResetForTest();
#endif

} // namespace SimGateway

#endif // ARDUINO_ARCH_RP2040
