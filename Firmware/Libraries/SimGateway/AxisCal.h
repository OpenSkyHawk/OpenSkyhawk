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

} // namespace OpenSkyhawk
