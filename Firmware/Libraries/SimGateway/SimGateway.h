/**
 * @file SimGateway.h
 * @brief RP2040 USB HID gateway library for OpenSkyhawk SimGateway board.
 *
 * Owns the USB CDC ↔ UART relay, 0xAA 0x55 HID frame demultiplexer, and
 * HID report batching. HIDAxis, HIDButton, and HIDHatSwitch objects are
 * declared in the sketch at file scope and self-register into linked lists at
 * construction. SimGateway::loop() walks those lists and dispatches matching HID
 * frames into the USB HID report.
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

#include "AxisCal.h"
#include "LinkRecovery.h"

namespace OpenSkyhawk {

/**
 * @brief HID axis handler. Declared at sketch scope for each joystick axis.
 *
 * Self-registers into a static linked list at construction. SimGateway::loop()
 * walks the list and writes the calibrated value into the HID axis report when a frame with a matching
 * controlId arrives. The 0–65535 unsigned value from PanelGroup is mapped to
 * signed ±32767 internally (value − 32768).
 *
 * Declare at file scope, not inside functions — C++ constructs file-scope objects
 * before setup() runs, which is required for the linked list to be populated.
 *
 * **Calibration is never a constructor argument.** Endpoints are per-unit — sensor
 * spread, magnet geometry, print tolerance — so baking them in would mean a reflash
 * per build, which is exactly the limitation this design removes. They live in flash,
 * are loaded once by SimGateway::setup(), and are looked up by `axisIndex`. Sketch
 * declarations are therefore identical calibrated or not.
 */
class HIDAxis {
public:
    /**
     * @brief Register a HID axis handler.
     * @param controlId  CTRL_* constant from HIDControls.h (0x0010–0x001F range).
     * @param axisIndex  HID report axis index (0–7).
     */
    HIDAxis(uint16_t controlId, uint8_t axisIndex);

    static HIDAxis* head();       ///< First registered axis; nullptr if none.
    uint16_t controlId() const;   ///< controlId this handler is registered for.
    uint8_t  axisIndex() const;   ///< Joystick axis index (0–7) this handler drives.
    /**
     * @brief Dispatch a value to the joystick axis.
     * @param value  Unsigned 0–65535 as emitted by the node, pre-calibration.
     *
     * Applies the stored calibration first, then the fixed unsigned→signed offset.
     * Order matters: the pipeline stays unsigned until the final step, so the map
     * never divides on signed values.
     *
     * @note The −32768 offset is invariant, not calibration-dependent. Two different
     *       centres are in play: `AxisCal::centre` is the axis's *physical* rest point,
     *       which the map absorbs; −32768 is the re-centring of a 16-bit range, a
     *       property of the representation. axisCalApply() always returns 0–65535, so
     *       making the offset variable would apply the centring twice.
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
     * @param buttonIndex HID report button index (0–127).
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
 * Writes the HID hat nibble with direction clamped to 0–8.
 */
class HIDHatSwitch {
public:
    /**
     * @brief Register a HID hat switch handler.
     * @param controlId  CTRL_* constant from HIDControls.h (0x0020–0x002F range).
     * @param hatIndex   HID report hat index (0–3).
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

// Firmware version reported in the calibration protocol's HELLO_ACK, so the client can show
// which build a device is running. Keep in step with library.json.
#define SIMGATEWAY_FW_MAJOR 0
#define SIMGATEWAY_FW_MINOR 1
#define SIMGATEWAY_FW_PATCH 0

/**
 * @brief Initialise USB identity, the HID device, and the UART link to PanelBridge.
 *
 * Must be the first call in the sketch's setup(). Sets USB identity before the
 * TinyUSB stack enumerates:
 *   - Manufacturer: "OpenSkyhawk"
 *   - Product:      "A-4E Skyhawk"
 *   - VID/PID:      0x2E8A / 0x4134
 *   - CDC port:     "A-4E Skyhawk DCS-BIOS" (iInterface — names the serial port)
 * Configures the UART pins, enlarges the UART's software RX queue to UART_RX_QUEUE_BYTES and
 * calls uart.begin(250000), then loads the stored axis calibration and initialises the HID
 * descriptor, then enumerates. Also records why this boot happened (see diagnostics()).
 *
 * @param uart   Hardware UART connected to PanelBridge (Serial1 / UART0 on standard board).
 *               Must not have been begun already — the RX queue size can only be set before.
 * @param txPin  RP2040 UART TX pin. Defaults to GP0, wired to STM32 PA3.
 * @param rxPin  RP2040 UART RX pin. Defaults to GP1, wired to STM32 PA2.
 *
 * @note Calibration is loaded before the HID stack comes up, which matters only for
 *       tidiness — the load itself has no ordering requirement. Axis objects are
 *       constructed at static init, long before this runs, and they index a `.bss` blob
 *       that the C runtime has already zeroed. A zeroed blob fails axisCalValid(), so an
 *       axis dispatched before setup() would pass through unchanged rather than divide
 *       by zero.
 */
void setup(SerialUART& uart,
           uint8_t txPin = DEFAULT_UART_TX_PIN,
           uint8_t rxPin = DEFAULT_UART_RX_PIN);

/// Bytes the UART's software RX queue holds (core default 32). At 250000 baud that is ~40 ms
/// of loop() latency before bytes are lost, instead of ~1.3 ms.
static constexpr size_t UART_RX_QUEUE_BYTES = 1024;

/// Host-bound bytes per line the relay buffers. The longest real line is a `_NODE_STATUS`
/// row at ~42 bytes; anything longer is not a line PanelBridge sends, and is dropped.
static constexpr uint16_t RELAY_LINE_MAX = 128;

/// Host→PanelBridge bytes forwarded per loop() call. The UART write blocks once its 32-byte
/// hardware FIFO is full (~40 µs per byte), so an uncapped drain of a DCS export burst would
/// hold loop() for tens of ms. 64 bytes bounds it at ~2.6 ms.
static constexpr uint16_t CDC_TO_UART_BUDGET = 64;

/// The CDC IN endpoint counts as stalled after this long with DTR held, bytes pending and
/// nothing drained. The host OS keeps polling an open port into its own buffer even when the
/// application is not reading, so a FIFO that stays full this long means the host's USB stack
/// stopped polling — a configured-but-dead link that `mounted()` alone cannot see.
static constexpr uint32_t CDC_STALL_MS = 1000;

/// Hardware watchdog timeout. Armed by the first loop() call, fed by every call.
static constexpr uint32_t WATCHDOG_MS = 3000;

/**
 * @brief Relay bytes and dispatch HID frames. Call once per loop() iteration.
 *
 * Per call:
 *   1. Forward up to CDC_TO_UART_BUDGET USB CDC bytes (Serial) to UART — DCS-BIOS stream to
 *      PanelBridge.
 *   2. Drain all UART bytes:
 *        0xAA 0x55 + 4 bytes     → parse controlId + value LE; dispatch to HID lists
 *        any other byte          → append to the host-bound line; on `\n` the whole line is
 *                                  written to USB CDC, or dropped and counted (see below)
 *        0xAA + non-0x55         → both bytes go to the line, which makes it corrupt; resume IDLE
 *   3. If any HID setter fired, send the HID report exactly once.
 *   4. Status LEDs, UART error counters, FAULT self-clear.
 *   5. USB recovery ladder (LinkRecovery.h).
 *
 * **Never blocks on the host.** A line is written only if the CDC FIFO has room for all of
 * it; otherwise it is dropped whole — never truncated — and counted in
 * GatewayDiag::linesDropped. A line holding anything but printable ASCII (0x20–0x7E) and its
 * `\n` is dropped as corrupt, and one longer than RELAY_LINE_MAX as overlong. Everything
 * PanelBridge sends the host is a newline-framed DCS-BIOS message, so neither happens on a
 * healthy link. Calibration frames are written whole or not at all in the same way.
 *
 * @note The first call arms a WATCHDOG_MS hardware watchdog; after that, loop() must be called
 *       at least that often or the chip resets (production builds only).
 *
 * @note Node-status reporting (#86) needs no handling here: PanelBridge's `_NODE_STATUS` DCS-BIOS
 *       messages are ASCII (≤ 0x7F) forwarded by step 2, and the host's roster request is a
 *       DCS-BIOS export write (addr 0x86FE) forwarded by step 1. Both transit transparently.
 *
 * @note Parser state persists across calls — frames split across iterations assemble correctly.
 */
void loop();

/**
 * @brief Link-health counters and event times (#355). Reported to the host as `DIAG`.
 *
 * Times are millis() since this boot, 0 = never. Counters saturate at 0xFFFF. The field
 * meanings are the `CAL_DIAG_*` offsets in AxisCal.h, which define the wire layout.
 */
struct GatewayDiag {
    uint32_t firstUartErrMs;   ///< First PL011 error this boot.
    uint32_t lastUartErrMs;    ///< Latest PL011 error.
    uint32_t firstUsbLossMs;   ///< First healthy → unhealthy host-link edge.
    uint32_t lastUsbLossMs;    ///< Latest such edge.
    uint16_t uartOverrun;      ///< Status ticks with RSR OE set.
    uint16_t uartFraming;      ///< Status ticks with RSR FE set.
    uint16_t uartBreak;        ///< Status ticks with RSR BE set.
    uint16_t uartParity;       ///< Status ticks with RSR PE set.
    uint16_t uartRxOverflow;   ///< Status ticks on which the software RX queue had overflowed.
    uint16_t linesDropped;     ///< Lines dropped for lack of CDC room.
    uint16_t linesCorrupt;     ///< Lines dropped for a non-ASCII byte.
    uint16_t linesOverlong;    ///< Lines dropped for exceeding RELAY_LINE_MAX.
    uint16_t framesDropped;    ///< Calibration frames dropped for lack of CDC room.
    uint16_t uartRestarts;     ///< FAULT self-clears (UART restarted).
    uint16_t usbReattaches;    ///< Recovery-ladder detach/attach cycles.
    uint16_t cdcStalls;        ///< CDC stall onsets: DTR held, bytes pending, no drain for CDC_STALL_MS.
    uint8_t  recoveryReboots;  ///< Recovery-ladder reboots. Survives a warm reboot.
    uint8_t  watchdogResets;   ///< Hardware watchdog resets. Survives a warm reboot.
    uint8_t  resetReason;      ///< OpenSkyhawk::CalDiagResetReason for this boot.
};

/** @brief The live link-health counters. Safe to call at any time. */
const GatewayDiag& diagnostics();

/**
 * @brief The calibration currently applied to dispatched axis values.
 * @return The in-RAM blob, loaded by setup() from flash.
 *
 * Every axis reads as uncalibrated until setup() loads a valid blob, so this is safe to
 * call at any time. Use OpenSkyhawk::axisCalValid() on an element to ask whether that
 * particular axis is calibrated.
 */
const OpenSkyhawk::CalBlob& calibration();

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
    FAULT,     ///< uart0 hardware error (RSR) — RED FAST. ≥2 s hold; clears on clean data, or after 3 s by restarting the UART.
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

/** @brief Reset parser to IDLE and discard any part-assembled host-bound line (test builds only). */
void resetParser();

/**
 * @brief Pretend the CDC FIFO has `room` bytes free (test builds only).
 * @param room  Free bytes reported to the relay; negative = unlimited (the default).
 * @note The capture records only what the relay actually wrote, so a dropped line or frame
 *       leaves it untouched.
 */
void cdcSetRoomForTest(int32_t room);

/** @brief Zero diagnostics() (test builds only). */
void diagResetForTest();

/**
 * @brief Run the PL011 error accounting for one status tick (test builds only).
 * @param now  Fake millis() timestamp.
 * @param oe, fe, be, pe  Which RSR bits were set this tick.
 */
void diagUartErrorsForTest(uint32_t now, bool oe, bool fe, bool be, bool pe);

/**
 * @brief Run one tick of the USB recovery ladder with injected inputs (test builds only).
 * @param now        Fake millis() timestamp.
 * @param mounted    Fake TinyUSBDevice.mounted().
 * @param suspended  Fake TinyUSBDevice.suspended().
 * @return The action production would apply. Also updates the USB-loss times and the
 *         re-attach counter in diagnostics(), exactly as production does.
 * @note "Recent UART fault" comes from diagnostics().lastUartErrMs — drive it with
 *       diagUartErrorsForTest().
 */
OpenSkyhawk::LinkAction linkStepForTest(uint32_t now, bool mounted, bool suspended,
                                        bool cdcStalled = false);

/**
 * @brief Run the CDC stall detector one tick (test builds only).
 * @param now       Fake millis() timestamp.
 * @param dtr       Fake Serial.dtr() — the host has the port open.
 * @param room      Fake Serial.availableForWrite().
 * @param capacity  CDC TX FIFO size (room == capacity means nothing pending).
 * @param written   Bytes the relay wrote since the previous tick.
 * @return true when the endpoint counts as stalled (feeds linkStepForTest's `cdcStalled`).
 *         Counts diagnostics().cdcStalls on each onset.
 */
bool cdcStallStepForTest(uint32_t now, bool dtr, uint16_t room, uint16_t capacity, uint16_t written);

/** @brief Reset the recovery ladder and the CDC stall detector, as at boot (test builds only). */
void linkResetForTest(bool afterRecoveryReboot);

/** @brief Clear captured parser-forwarded USB CDC bytes (test builds only). */
void resetCdcCapture();

/** @brief Number of parser-forwarded USB CDC bytes captured since resetCdcCapture(). */
size_t cdcCaptureCount();

/** @brief Captured parser-forwarded USB CDC byte at index, or 0 if out of range. */
uint8_t cdcCaptureByte(size_t index);

/** @brief True if more CDC bytes were forwarded than the test capture buffer can hold. */
bool cdcCaptureOverflow();

// ── Axis calibration test hooks (test builds only) ────────────────────────────
// The transform itself is tested by calling OpenSkyhawk::axisCalApply() directly. These
// hooks exist to cover something that function-level test cannot: that HIDAxis::dispatch()
// actually *applies* it. That path is doubly hidden in test builds — the HID setters are
// no-op stubs, and the flash load is stubbed to a RAM clear so no calibration can be
// installed. Without both a writer and a reader, deleting the transform from dispatch()
// would leave every assertion passing.

/**
 * @brief Install a calibration blob directly, bypassing flash (test builds only).
 * @param blob  Blob to make live. Not validated — tests may install deliberately bad values.
 */
void calSetForTest(const OpenSkyhawk::CalBlob& blob);

/** @brief Value last written to the HID axis report — post-calibration, post-offset. */
int16_t lastAxisValue();

/** @brief Axis index last written to the HID axis report; 0xFF if none since reset. */
uint8_t lastAxisIndex();

/** @brief Clear the captured axis write. Call between test cases. */
void resetAxisCapture();

/**
 * @brief Number of HID report sends since resetHidSendCount().
 * @note The batching contract is one report per loop() drain. Nothing else observes report
 *       sends, so this is the only way to assert that a dispatch-path change did not add
 *       sends of its own.
 */
uint8_t hidSendCount();

/** @brief Clear the report-send counter. Call between test cases. */
void resetHidSendCount();

// ── Calibration transport test hooks (test builds only) ───────────────────────
// The transport's safety property is that a rejected candidate is handed back to the relay
// byte-for-byte. Asserting that needs a capture on the UART side, which had no test seam.

/**
 * @brief Feed one inbound (host→device) byte, exactly as loop() step 1 would.
 * @return true if the calibration transport absorbed it; false if it was relayed onward.
 * @note Relaying is included, so the UART capture reflects what PanelBridge would actually
 *       receive. A hook that only ran the parser would let a test pass while the bytes it
 *       declined went nowhere.
 */
bool feedCdcByte(uint8_t b);

/** @brief Clear the captured UART-bound bytes. Call between test cases. */
void resetUartCapture();

/** @brief Number of bytes relayed toward PanelBridge since resetUartCapture(). */
size_t uartCaptureCount();

/** @brief Captured UART-bound byte at index, or 0 if out of range. */
uint8_t uartCaptureByte(size_t index);

/** @brief True if more bytes were relayed than the capture buffer can hold. */
bool uartCaptureOverflow();

/** @brief Clear session state and the partially-received frame. Call between test cases. */
void calResetForTest();

/** @brief True while a calibration session is open. */
bool calSessionOpen();

/** @brief Axis currently selected for RAW streaming, or 0xFF for none. */
uint8_t calStreamAxis();

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

/**
 * @brief Run the FAULT self-clear check one tick (test builds). Pairs with statusFaultStep().
 * @param now  Fake millis() timestamp.
 * @return true if FAULT had been latched for FAULT_RESTART_MS: the latch is cleared and
 *         production would restart the UART.
 */
bool statusFaultRestartCheck(uint32_t now);

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
