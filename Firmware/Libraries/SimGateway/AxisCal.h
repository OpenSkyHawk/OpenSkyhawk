/**
 * @file AxisCal.h
 * @brief Per-axis HID calibration — storage layout, the two-segment transform, and CRC.
 *
 * A hall-effect stick spans only part of the ADC range (57–62% on the bench rig), so an
 * uncalibrated axis reaches roughly ±20k instead of ±32767. Its neutral position is also
 * rarely at the midpoint of that span, which a single linear map cannot correct: stretching
 * min..max fixes the range but leaves the stick resting off-centre.
 *
 * This is the fix — three captured points (min, centre, max) and two linear segments, so
 * centre lands exactly on 32768 regardless of how asymmetric the mechanical travel is.
 *
 * Everything here is **pure**: no EEPROM, no Serial, no millis(), no globals, no hardware.
 * That is deliberate. SimGateway's `_hidSetAxis` is a no-op stub in SIMGATEWAY_TEST builds,
 * so a transform reachable only through HIDAxis::dispatch() would be unobservable in tests;
 * a free function taking its inputs as arguments is callable directly from a test sketch.
 *
 * Deliberately not wrapped in `#ifdef ARDUINO_ARCH_RP2040` — this is portable C++ and the
 * guard would only obstruct compiling it anywhere else.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace OpenSkyhawk {

/**
 * @brief Captured endpoints for one axis, unsigned 0–65535 throughout.
 *
 * `centre` is always *provided* data, never derived — the client captures it, and neither the
 * device nor the client substitutes `(min + max) / 2`. Raw midpoint is not physical midpoint
 * whenever the transfer is nonlinear, and for a rotating-magnet hall axis it is nonlinear by
 * construction (the normal field component goes as sin θ). An offset centre is the normal
 * case and is exactly what the second segment exists to absorb.
 */
struct AxisCal {
    uint16_t min;       ///< Raw value at the axis's low mechanical stop.
    uint16_t centre;    ///< Raw value at rest. Captured, never computed.
    uint16_t max;       ///< Raw value at the axis's high mechanical stop.
    /**
     * Reserved, always 0 in blob version 1. The field exists only so that adding a deadzone
     * later needs no CAL_VERSION bump; nothing reads it, and the wire protocol rejects a
     * non-zero value. AnalogInput's 128-count output hysteresis already hides return scatter,
     * and if scatter ever exceeds that the fix is that axis's `hysteresis` argument, not a
     * user-facing control.
     */
    uint16_t deadzone;
};

/// HID report axis slots. Fixed by the report descriptor, not by how many a cockpit populates.
constexpr uint8_t AXIS_CAL_SLOTS = 8;

/// Blob signature. Little-endian in flash, so a hexdump reads "OSKC".
constexpr uint32_t CAL_MAGIC = ((uint32_t)'O')
                             | ((uint32_t)'S' <<  8)
                             | ((uint32_t)'K' << 16)
                             | ((uint32_t)'C' << 24);

/**
 * @brief Blob layout version.
 * @note A mismatch means "absent" — every axis falls back to identity and flash is left
 *       untouched until the user commits. No migration and no auto-rewrite: a
 *       downgrade-then-upgrade cycle would silently destroy data.
 */
constexpr uint16_t CAL_VERSION = 1;

/**
 * @brief The whole persisted calibration set, written and erased as one unit.
 *
 * Offsets are natural-alignment under ARM EABI and happen to pack tight with no padding.
 * That is luck rather than design, which is why the static_asserts below are load-bearing —
 * `crc` is computed over `offsetof(CalBlob, crc)` bytes, so a layout change silently
 * invalidates every stored blob unless CAL_VERSION moves with it.
 *
 * @note Not `__attribute__((packed))`. There is no padding to remove, and packing would make
 *       every field access unaligned on a Cortex-M0+, which has no unaligned load/store.
 */
struct CalBlob {
    uint32_t magic;                  ///< offset  0 — CAL_MAGIC
    uint16_t version;                ///< offset  4 — CAL_VERSION
    AxisCal  axes[AXIS_CAL_SLOTS];   ///< offset  6 — 64 bytes
    uint16_t crc;                    ///< offset 70 — CRC-16/CCITT-FALSE over bytes [0, 70)
};

static_assert(sizeof(AxisCal) == 8,  "AxisCal layout changed — bump CAL_VERSION");
static_assert(sizeof(CalBlob) == 72, "CalBlob layout changed — bump CAL_VERSION");
static_assert(offsetof(CalBlob, crc) == 70, "CRC coverage is offsetof(crc); layout changed");

/**
 * @brief CRC-16/CCITT-FALSE — poly 0x1021, init 0xFFFF, no reflection, no final XOR.
 * @param data  Bytes to cover.
 * @param len   Byte count.
 * @return The CRC.
 * @note Canonical check: "123456789" → 0x29B1. Init is 0xFFFF rather than 0x0000 so that
 *       leading zero bytes change the result — an all-zero blob is a realistic corruption
 *       mode, and a 0x0000 init would not distinguish it from a shorter all-zero one.
 *       Bitwise and table-free: ~70 bytes of input costs a few microseconds, irrelevant
 *       beside the ~45 ms sector erase it protects.
 */
uint16_t calCrc16(const uint8_t* data, size_t len);

/**
 * @brief True when both segments have a non-zero divisor, i.e. the axis is calibrated.
 * @param cal  Endpoints to check.
 * @return true if `min < centre < max` allowing for the deadzone.
 * @note This doubles as the calibrated/uncalibrated predicate — there is no stored validity
 *       flag, because a flag could only duplicate or contradict this. An all-zero blob
 *       (.bss, never loaded) and an all-0xFF blob (erased flash) both fail it, so both fail
 *       closed to identity.
 * @note Widened to uint32_t deliberately. The obvious `min < centre - deadzone` underflows on
 *       uint16_t when deadzone > centre. The wire protocol rejects a non-zero deadzone, but
 *       this guard's whole job is preventing a divide-by-zero in axisCalApply(), so it must
 *       not depend on a check one layer up.
 */
bool axisCalValid(const AxisCal& cal);

/**
 * @brief Map a raw axis reading through the two-segment calibration.
 * @param cal  Endpoints for this axis.
 * @param raw  Unsigned 0–65535 as emitted by the node.
 * @return Unsigned 0–65535, with `cal.centre` landing exactly on 32768.
 *
 * Returns `raw` unchanged when the axis is uncalibrated, so an unwritten blob behaves
 * identically to a build without this feature.
 *
 * @note Integer only, by design — the arithmetic must be reproducible exactly, and a curve
 *       would stack with the per-aircraft curves DCS already applies.
 * @note The uint32_t cast must sit on the multiply *operand*. Worst case is
 *       65534 × 32768 = 2 147 418 112, inside uint32_t with 2× headroom. `int` is 32-bit on
 *       RP2040 so the wrong form would work here by accident; the explicit cast is the
 *       portable contract.
 * @note 65535 is reachable only via the upper clamp — the upper segment's arithmetic tops out
 *       at 65534. Both map to +32767 after the caller's −32768, so this is correct, but it is
 *       the kind of asymmetry someone will otherwise "fix".
 */
uint16_t axisCalApply(const AxisCal& cal, uint16_t raw);

/**
 * @brief CRC of a blob's covered region, i.e. everything before the `crc` field itself.
 * @param blob  Blob to checksum.
 * @return The CRC to store in, or compare against, `blob.crc`.
 */
uint16_t calBlobCrc(const CalBlob& blob);

/**
 * @brief True when a blob carries the right signature, version, and checksum.
 * @param blob  Blob as read from storage.
 * @return true if the blob should be trusted.
 */
bool calBlobValid(const CalBlob& blob);

/**
 * @brief Zero a blob so every axis reads as uncalibrated.
 * @param blob  Blob to clear.
 * @note Clears RAM only. Nothing here writes flash.
 */
void calBlobClear(CalBlob& blob);

/**
 * @brief Stamp magic, version, and a fresh CRC onto a blob ahead of persisting it.
 * @param blob  Blob whose `axes` are already populated.
 */
void calBlobSeal(CalBlob& blob);

// ── Calibration wire protocol ─────────────────────────────────────────────────
//
// The USB CDC protocol between SimGateway and SkyHawkClient. Specified in
// FirmwarePlan/03-uart-usb-hid-protocol.md § Calibration Protocol (USB CDC), which is
// authoritative — if this header disagrees with that page, the page wins.
//
// Only the pure parts live here: the length table, the frame builder, and whole-frame
// validation. The byte-at-a-time receive state machine belongs to SimGateway.cpp because
// it has to re-emit rejected bytes into the relay.

constexpr uint8_t CAL_PROTO_VERSION = 1;

/// Frame lead-in. 0xAA leads all non-DCS data on this link, matching the HID frame magic.
/// Distinct from CAL_MAGIC above, which signs the stored blob rather than a wire frame.
constexpr uint8_t CAL_FRAME_MAGIC[4] = { 0xAA, 0x53, 0x4B, 0x43 };   // "\xAA S K C"

constexpr uint16_t CAL_ENVELOPE_BYTES = 10;   ///< magic 4 + type 1 + seq 1 + len 2 + crc 2
constexpr uint16_t CAL_MAX_PAYLOAD    = 82;   ///< CAL_DATA, the largest legal payload
constexpr uint16_t CAL_MAX_FRAME      = CAL_ENVELOPE_BYTES + CAL_MAX_PAYLOAD;  // 92

/// Message types. High bit set = device→client, so direction is readable in a capture.
enum CalType : uint8_t {
    CAL_T_HELLO         = 0x01,
    CAL_T_GET_CAL       = 0x02,
    CAL_T_SESSION_OPEN  = 0x03,
    CAL_T_SESSION_CLOSE = 0x04,
    CAL_T_COMMIT        = 0x05,
    CAL_T_RESET         = 0x06,
    CAL_T_KEEPALIVE     = 0x07,
    CAL_T_STREAM_SELECT = 0x08,
    CAL_T_GET_DIAG      = 0x09,   // link-health counters (#355); answered at any time

    CAL_T_HELLO_ACK     = 0x81,
    CAL_T_CAL_DATA      = 0x82,
    CAL_T_SESSION_ACK   = 0x83,
    CAL_T_ACK           = 0x84,
    CAL_T_NACK          = 0x85,
    CAL_T_RAW           = 0x86,
    CAL_T_DIAG          = 0x87,
};

/// NACK reasons. `detail` names the offending axis where one applies, else 0xFF.
enum CalNackReason : uint8_t {
    CAL_NACK_BAD_CRC      = 0x01,
    CAL_NACK_BAD_LENGTH   = 0x02,
    CAL_NACK_BAD_TYPE     = 0x03,
    CAL_NACK_BAD_INDEX    = 0x04,
    CAL_NACK_BAD_ORDER    = 0x05,
    CAL_NACK_NO_SESSION   = 0x06,
    CAL_NACK_NO_STORAGE   = 0x07,
    CAL_NACK_BAD_DEADZONE = 0x08,
};

/// `RESET` and the axis-selection fields use this to mean "all" / "none".
constexpr uint8_t CAL_AXIS_NONE = 0xFF;

// ── Payload field offsets ─────────────────────────────────────────────────────
//
// Byte offsets into each type's payload (not the frame — the payload starts after the 8-byte
// header). Multi-byte fields are little-endian, matching the envelope's LEN. A field runs to the
// next offset; the last runs to the type's length in calLenValidForType(). Types with no payload
// have no enum. Named so the client sync generates them rather than hand-typing them (#346).

/// `HELLO_ACK` payload (6 bytes, device→client).
enum CalHelloAckField : uint8_t {
    CAL_HELLO_ACK_PROTO        = 0,  // u8: CAL_PROTO_VERSION
    CAL_HELLO_ACK_BLOB_VERSION = 1,  // u8: CAL_VERSION
    CAL_HELLO_ACK_AXIS_SLOTS   = 2,  // u8: AXIS_CAL_SLOTS
    CAL_HELLO_ACK_FW_MAJOR     = 3,  // u8: SimGateway firmware version
    CAL_HELLO_ACK_FW_MINOR     = 4,  // u8
    CAL_HELLO_ACK_FW_PATCH     = 5,  // u8
};

/// `CAL_DATA` payload (82 bytes, device→client): two bitmasks, then AXIS_CAL_SLOTS records.
enum CalDataField : uint8_t {
    CAL_DATA_PRESENT_MASK    = 0,  // u8: bit i = an HIDAxis uses slot i
    CAL_DATA_CALIBRATED_MASK = 1,  // u8: bit i = slot i passes axisCalValid()
    CAL_DATA_RECORDS         = 2,  // AXIS_CAL_SLOTS records of CAL_DATA_REC_BYTES, slot order
};

/// One `CAL_DATA` record's length in bytes.
constexpr uint8_t CAL_DATA_REC_BYTES = 10;

/// Field offsets within one `CAL_DATA` record (record i starts at CAL_DATA_RECORDS + i × 10).
enum CalDataRecField : uint8_t {
    CAL_DATA_REC_CONTROL_ID = 0,  // u16: HID controlId of the axis in this slot, 0 = absent
    CAL_DATA_REC_MIN        = 2,  // u16: AxisCal::min
    CAL_DATA_REC_CENTRE     = 4,  // u16: AxisCal::centre
    CAL_DATA_REC_MAX        = 6,  // u16: AxisCal::max
    CAL_DATA_REC_DEADZONE   = 8,  // u16: AxisCal::deadzone, always 0
};

/// `SESSION_OPEN` payload (1 byte, client→device).
enum CalSessionOpenField : uint8_t {
    CAL_SESSION_OPEN_AXIS_IDX = 0,  // u8: axis to stream, CAL_AXIS_NONE = none
};

/// `SESSION_ACK` payload (5 bytes, device→client).
enum CalSessionAckField : uint8_t {
    CAL_SESSION_ACK_TIMEOUT_MS = 0,  // u32: session inactivity timeout, ms
    CAL_SESSION_ACK_AXIS_IDX   = 4,  // u8: echoed streamed-axis selection
};

/// `STREAM_SELECT` payload (1 byte, client→device).
enum CalStreamSelectField : uint8_t {
    CAL_STREAM_SELECT_AXIS_IDX = 0,  // u8: axis to stream, CAL_AXIS_NONE = none
};

/// `COMMIT` payload (9 bytes, client→device) — exactly one axis.
enum CalCommitField : uint8_t {
    CAL_COMMIT_IDX      = 0,  // u8: axis slot
    CAL_COMMIT_MIN      = 1,  // u16
    CAL_COMMIT_CENTRE   = 3,  // u16
    CAL_COMMIT_MAX      = 5,  // u16
    CAL_COMMIT_DEADZONE = 7,  // u16: must be 0, else NACK BAD_DEADZONE
};

/// `RESET` payload (1 byte, client→device).
enum CalResetField : uint8_t {
    CAL_RESET_IDX = 0,  // u8: axis slot, CAL_AXIS_NONE = all
};

/// `ACK` payload (1 byte, device→client).
enum CalAckField : uint8_t {
    CAL_ACK_TYPE = 0,  // u8: CalType being acknowledged
};

/// `NACK` payload (3 bytes, device→client).
enum CalNackField : uint8_t {
    CAL_NACK_TYPE   = 0,  // u8: CalType being refused
    CAL_NACK_REASON = 1,  // u8: CalNackReason
    CAL_NACK_DETAIL = 2,  // u8: offending axis slot, else CAL_AXIS_NONE
};

/// `RAW` payload (5 bytes, device→client).
enum CalRawField : uint8_t {
    CAL_RAW_IDX = 0,  // u8: streamed axis slot
    CAL_RAW_RAW = 1,  // u16: reading before calibration
    CAL_RAW_CAL = 3,  // u16: the same reading through axisCalApply()
};

/// `DIAG` payload (47 bytes, device→client): link-health counters and event times (#355).
///
/// Times are millis() since this boot; 0 means "never". Counters are u16 and saturate at
/// 0xFFFF rather than wrap, so a large number never reads as a small one. The four UART error
/// counters count status ticks on which that PL011 RSR bit was set — the core's UART IRQ owns
/// the data register, so per-character counts are not available. The three u8 fields at the
/// end survive a warm reboot (watchdog scratch registers) and reset on power-on.
enum CalDiagField : uint8_t {
    CAL_DIAG_UPTIME_MS          = 0,   // u32: millis() when the reply was built
    CAL_DIAG_FIRST_UART_ERR_MS  = 4,   // u32: first PL011 error this boot
    CAL_DIAG_LAST_UART_ERR_MS   = 8,   // u32: latest PL011 error
    CAL_DIAG_FIRST_USB_LOSS_MS  = 12,  // u32: first healthy → unhealthy host-link edge
    CAL_DIAG_LAST_USB_LOSS_MS   = 16,  // u32: latest such edge
    CAL_DIAG_UART_OVERRUN       = 20,  // u16: RSR OE — the UART IRQ itself was held off
    CAL_DIAG_UART_FRAMING       = 22,  // u16: RSR FE — electrical
    CAL_DIAG_UART_BREAK         = 24,  // u16: RSR BE — RX held low (PanelBridge reset/off)
    CAL_DIAG_UART_PARITY        = 26,  // u16: RSR PE
    CAL_DIAG_UART_RX_OVERFLOW   = 28,  // u16: software RX queue full — loop() fell behind
    CAL_DIAG_LINES_DROPPED      = 30,  // u16: whole line dropped, no CDC room (host not reading)
    CAL_DIAG_LINES_CORRUPT      = 32,  // u16: line held a non-ASCII byte, not relayed
    CAL_DIAG_LINES_OVERLONG     = 34,  // u16: line exceeded the relay buffer, not relayed
    CAL_DIAG_FRAMES_DROPPED     = 36,  // u16: calibration frame dropped, no CDC room
    CAL_DIAG_UART_RESTARTS      = 38,  // u16: FAULT held too long → UART restarted
    CAL_DIAG_USB_REATTACHES     = 40,  // u16: recovery ladder detach/attach cycles
    CAL_DIAG_CDC_STALLS         = 42,  // u16: host held the port but the CDC IN endpoint stopped draining
    CAL_DIAG_RECOVERY_REBOOTS   = 44,  // u8:  recovery ladder reboots (survives reboot)
    CAL_DIAG_WATCHDOG_RESETS    = 45,  // u8:  hardware watchdog resets (survives reboot)
    CAL_DIAG_RESET_REASON       = 46,  // u8:  CalDiagResetReason for this boot
};

/// `DIAG` payload length.
constexpr uint8_t CAL_DIAG_BYTES = 47;

/// Why this boot happened, as reported in `DIAG`.
enum CalDiagResetReason : uint8_t {
    CAL_DIAG_RESET_POWER_ON = 0,  // cold start, RUN pin, or anything not listed below
    CAL_DIAG_RESET_WATCHDOG = 1,  // hardware watchdog: loop() stopped running
    CAL_DIAG_RESET_RECOVERY = 2,  // the USB recovery ladder rebooted on purpose
};

static_assert(CAL_DIAG_BYTES <= CAL_MAX_PAYLOAD, "DIAG must fit the largest legal payload");

static_assert(CAL_DATA_RECORDS + AXIS_CAL_SLOTS * CAL_DATA_REC_BYTES == CAL_MAX_PAYLOAD,
              "CAL_DATA layout no longer fills CAL_MAX_PAYLOAD");

/**
 * @brief Is `len` the only length this `type` may legally carry?
 * @param type  Message type byte.
 * @param len   Candidate payload length, as read off the wire.
 * @return true if the pair is legal.
 *
 * This is the framing-layer gate, and it is checked **before the payload is buffered**.
 * `len` is read before the CRC can be verified, so on a false frame it is noise: a stray
 * magic in DCS-BIOS text can decode a length near 65535, and a receiver that waits for that
 * many bytes stalls. Every type therefore has an exact length rather than a shared bound.
 *
 * @note There are no variable-length types. `COMMIT` carries exactly one axis, so the rule is
 *       uniform: one legal length per type, no exception to state or to get wrong. An earlier
 *       draft let `COMMIT` batch up to eight axes, which allowed a batch to name the same axis
 *       twice with different values and silently apply the last.
 * @note An unknown type is rejected. Protocol versions must match — `HELLO_ACK` carries
 *       `proto` for exactly that — so an unrecognised type is an error, not something to skip.
 */
bool calLenValidForType(uint8_t type, uint16_t len);

/**
 * @brief Build a complete frame into a caller-supplied buffer.
 * @param out      Destination, at least `CAL_ENVELOPE_BYTES + len` bytes.
 * @param outCap   Capacity of `out`.
 * @param type     Message type.
 * @param seq      Sequence byte — echoed from the request, or a counter for unsolicited RAW.
 * @param payload  Payload bytes; may be nullptr when `len` is 0.
 * @param len      Payload length. Must satisfy calLenValidForType().
 * @return Bytes written, or 0 if the arguments are inconsistent or `out` is too small.
 */
uint16_t calBuildFrame(uint8_t* out, uint16_t outCap,
                       uint8_t type, uint8_t seq,
                       const uint8_t* payload, uint16_t len);

/**
 * @brief Verify the CRC of a complete, already-assembled frame.
 * @param frame  Whole frame including magic and trailing CRC.
 * @param n      Frame length in bytes.
 * @return true if the trailing CRC matches the computed one.
 * @note Coverage is `TYPE`‖`SEQ`‖`LEN`‖`PAYLOAD` — the magic is excluded, and so is the CRC
 *       field itself. Checksumming constant bytes adds no detection power.
 */
bool calFrameCrcOk(const uint8_t* frame, uint16_t n);

} // namespace OpenSkyhawk
