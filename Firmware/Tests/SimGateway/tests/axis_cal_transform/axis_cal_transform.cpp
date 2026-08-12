// SimGateway - axis calibration transform test
//
// Exercises the two-segment calibration map, its validity predicate, and the blob
// checksum (issue #251). Everything here is a direct call into AxisCal — no UART, no
// HID stack, no flash. That is the reason the transform is a free function rather than
// a HIDAxis member: _hidSetAxis is a no-op stub in SIMGATEWAY_TEST builds, so a member
// transform could only ever be asserted indirectly.
//
// The endpoints below are real, measured on the AxisBench rig over a 30 s full-travel
// sweep of a PS5-style hall thumbstick. Roll matters most: its centre sits 7.1% above
// the midpoint of its travel, which is exactly the case a single linear map cannot fix.
//
// Covers:
//   - centre lands on 32768 exactly, and therefore on 0 after dispatch()'s offset
//   - clamping at and beyond both rails
//   - a symmetric axis is bit-exact identity across the whole range
//   - degenerate endpoints where a segment has no interior value
//   - integer truncation at the boundaries either side of centre
//   - validity rejects every fail-closed case, including the deadzone underflow
//   - CRC-16/CCITT-FALSE canonical vector, and that the CRC excludes its own field
//
// Flash:
//   pio run -e test_axis_cal_transform -t upload
// Monitor: open USB CDC (115200) on the Pico.

#include <Arduino.h>
#include <SimGateway.h>

using OpenSkyhawk::AxisCal;
using OpenSkyhawk::CalBlob;

static bool g_allPass = true;

static void check(const __FlashStringHelper* label, bool cond) {
    Serial.print(label);
    Serial.println(cond ? F(": PASS") : F(": FAIL"));
    g_allPass &= cond;
}

// Report the actual value on failure — a bare FAIL on an arithmetic test tells you nothing.
static void checkEq(const __FlashStringHelper* label, uint32_t got, uint32_t want) {
    const bool ok = (got == want);
    Serial.print(label);
    if (ok) {
        Serial.println(F(": PASS"));
    } else {
        Serial.print(F(": FAIL (got "));
        Serial.print(got);
        Serial.print(F(", want "));
        Serial.print(want);
        Serial.println(F(")"));
    }
    g_allPass &= ok;
}

// Measured on the AxisBench hall thumbstick, unsigned 16-bit.
static const AxisCal ROLL  = { 13443, 34728, 50704, 0 };   // 56.9% travel, centre +7.1%
static const AxisCal PITCH = { 13058, 33112, 53741, 0 };   // 62.1% travel, centre -0.7%
static const AxisCal SYMM  = {     0, 32768, 65535, 0 };   // ideal axis

// ── Centre, rails and clamping ────────────────────────────────────────────────
static void testCentreAndRails() {
    // The whole point of the third captured value. Under a single linear map this axis
    // rested at +4669 on the bench; here it must be dead centre.
    checkEq(F("[A1] Roll centre -> 32768"),  axisCalApply(ROLL,  ROLL.centre),  32768);
    checkEq(F("[A2] Pitch centre -> 32768"), axisCalApply(PITCH, PITCH.centre), 32768);

    // ...and therefore exactly 0 once dispatch() applies the fixed unsigned->signed offset.
    checkEq(F("[A3] Roll centre -> 0 after offset"),
            (uint32_t)(int32_t)(int16_t)(axisCalApply(ROLL, ROLL.centre) - 32768), 0);

    checkEq(F("[A4] Roll min -> 0"),      axisCalApply(ROLL, ROLL.min),  0);
    checkEq(F("[A5] Roll max -> 65535"),  axisCalApply(ROLL, ROLL.max),  65535);

    // Beyond the stops. Reachable in practice: a magnet can be nudged after calibration.
    checkEq(F("[A6] below min clamps"),   axisCalApply(ROLL, 0),     0);
    checkEq(F("[A7] above max clamps"),   axisCalApply(ROLL, 65535), 65535);
    checkEq(F("[A8] well above max clamps"), axisCalApply(ROLL, 60000), 65535);

    // The uncalibrated ~20k ceiling this feature exists to remove: before calibration the
    // rail read 50704 straight through, which is +17936 after the offset, not +32767.
    checkEq(F("[A9] Roll rail reaches full scale"),
            (uint32_t)(int32_t)(int16_t)(axisCalApply(ROLL, ROLL.max) - 32768), 32767);
}

// ── Truncation either side of centre ──────────────────────────────────────────
static void testTruncation() {
    // One count inside each stop, and one count either side of centre. These are the rows
    // where integer division actually bites; they pin the arithmetic against a rewrite.
    checkEq(F("[B1] Roll min+1"),      axisCalApply(ROLL, 13444), 1);
    checkEq(F("[B2] Roll centre-1"),   axisCalApply(ROLL, 34727), 32766);
    checkEq(F("[B3] Roll centre+1"),   axisCalApply(ROLL, 34729), 32770);
    checkEq(F("[B4] Roll max-1"),      axisCalApply(ROLL, 50703), 65532);

    // Monotonic across the seam: the map must never step backwards at centre.
    bool monotonic = true;
    uint16_t prev = 0;
    for (uint32_t raw = ROLL.min; raw <= ROLL.max; raw += 37) {
        const uint16_t v = axisCalApply(ROLL, (uint16_t)raw);
        if (v < prev) monotonic = false;
        prev = v;
    }
    check(F("[B5] monotonic non-decreasing across centre"), monotonic);
}

// ── A symmetric axis must be exact identity ───────────────────────────────────
static void testSymmetricIsIdentity() {
    // Proves the transform introduces no error of its own on an already-perfect axis.
    // Stepping by 7 hits both segments and lands off every power-of-two boundary.
    bool identity = true;
    uint16_t firstBad = 0;
    for (uint32_t raw = 0; raw <= 65535; raw += 7) {
        if (axisCalApply(SYMM, (uint16_t)raw) != (uint16_t)raw) {
            identity = false;
            firstBad = (uint16_t)raw;
            break;
        }
    }
    if (!identity) {
        Serial.print(F("       first mismatch at raw="));
        Serial.print(firstBad);
        Serial.print(F(" -> "));
        Serial.println(axisCalApply(SYMM, firstBad));
    }
    check(F("[C1] symmetric axis is exact identity"), identity);

    // The endpoints the sweep's stride skips.
    checkEq(F("[C2] symmetric 65535"), axisCalApply(SYMM, 65535), 65535);
    checkEq(F("[C3] symmetric 32768"), axisCalApply(SYMM, 32768), 32768);
}

// ── Degenerate endpoints ──────────────────────────────────────────────────────
static void testDegenerate() {
    // centre == min + 1: the lower segment has no interior value at all, so the branch
    // can never be entered with a zero-width range. Valid, and must not divide by zero.
    const AxisCal lowStub = { 0, 1, 65535, 0 };
    check   (F("[D1] centre==min+1 is valid"), axisCalValid(lowStub));
    checkEq (F("[D2] centre==min+1 at min"),    axisCalApply(lowStub, 0),     0);
    checkEq (F("[D3] centre==min+1 at centre"), axisCalApply(lowStub, 1),     32768);
    checkEq (F("[D4] centre==min+1 at max"),    axisCalApply(lowStub, 65535), 65535);

    // centre == max - 1: same, for the upper segment.
    const AxisCal highStub = { 0, 65534, 65535, 0 };
    check   (F("[D5] centre==max-1 is valid"), axisCalValid(highStub));
    checkEq (F("[D6] centre==max-1 at centre"), axisCalApply(highStub, 65534), 32768);
    checkEq (F("[D7] centre==max-1 at max"),    axisCalApply(highStub, 65535), 65535);
    checkEq (F("[D8] centre==max-1 below"),     axisCalApply(highStub, 65533), 32767);
}

// ── Validity: everything must fail closed ─────────────────────────────────────
static void testValidity() {
    check(F("[E1] Roll is valid"),  axisCalValid(ROLL));
    check(F("[E2] Pitch is valid"), axisCalValid(PITCH));

    // .bss before setup() runs. Must fail, or every axis divides by zero at static init.
    const AxisCal zeroed = { 0, 0, 0, 0 };
    check(F("[E3] all-zero is invalid"), !axisCalValid(zeroed));

    // Erased flash. Must fail, or a blank device looks calibrated.
    const AxisCal erased = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
    check(F("[E4] all-0xFF is invalid"), !axisCalValid(erased));

    const AxisCal minEqCentre = { 100, 100, 200, 0 };
    const AxisCal centreEqMax = { 100, 200, 200, 0 };
    const AxisCal inverted    = { 200, 150, 100, 0 };
    check(F("[E5] min==centre is invalid"), !axisCalValid(minEqCentre));
    check(F("[E6] centre==max is invalid"), !axisCalValid(centreEqMax));
    check(F("[E7] min>max is invalid"),     !axisCalValid(inverted));

    // The underflow guard. On uint16_t, `centre - deadzone` wraps to 5546 here and the
    // check would wrongly pass; widening to uint32_t is what makes this fail closed.
    const AxisCal dzWrap = { 0, 10, 20, 60000 };
    check(F("[E8] deadzone underflow is invalid"), !axisCalValid(dzWrap));

    // An invalid axis passes its input through untouched — an unwritten blob must behave
    // exactly like a build without this feature.
    checkEq(F("[E9] invalid axis is identity"),   axisCalApply(zeroed, 12345), 12345);
    checkEq(F("[E10] invalid axis passes rails"), axisCalApply(erased, 65535), 65535);
}

// ── CRC and blob framing ──────────────────────────────────────────────────────
static void testBlob() {
    // Canonical CCITT-FALSE vector. Pins poly, init, reflection and final XOR in one shot.
    static const char kCheck[] = "123456789";
    checkEq(F("[F1] CRC \"123456789\" == 0x29B1"),
            OpenSkyhawk::calCrc16((const uint8_t*)kCheck, 9), 0x29B1);

    checkEq(F("[F2] sizeof(CalBlob) == 72"), sizeof(CalBlob), 72);
    checkEq(F("[F3] crc at offset 70"), offsetof(CalBlob, crc), 70);

    CalBlob blob;
    OpenSkyhawk::calBlobClear(blob);
    check(F("[F4] cleared blob is invalid"), !OpenSkyhawk::calBlobValid(blob));

    blob.axes[0] = ROLL;
    blob.axes[1] = PITCH;
    OpenSkyhawk::calBlobSeal(blob);
    check(F("[F5] sealed blob is valid"), OpenSkyhawk::calBlobValid(blob));
    check(F("[F6] sealed blob keeps its axes"),
          axisCalValid(blob.axes[0]) && axisCalValid(blob.axes[1]) &&
          !axisCalValid(blob.axes[2]));

    // The CRC must not cover its own field, or no blob could ever validate. Corrupting
    // only `crc` changes the stored value but not the computed one.
    const uint16_t computed = OpenSkyhawk::calBlobCrc(blob);
    blob.crc ^= 0x0001;
    check(F("[F7] CRC excludes its own field"), OpenSkyhawk::calBlobCrc(blob) == computed);
    check(F("[F8] corrupt CRC field rejected"), !OpenSkyhawk::calBlobValid(blob));
    blob.crc ^= 0x0001;

    // Single-bit flip anywhere in the payload.
    blob.axes[0].centre ^= 0x0001;
    check(F("[F9] single-bit payload flip rejected"), !OpenSkyhawk::calBlobValid(blob));
    blob.axes[0].centre ^= 0x0001;
    check(F("[F10] restored blob validates again"), OpenSkyhawk::calBlobValid(blob));

    // Re-checksum after each header edit, so these assert the magic and version checks
    // specifically rather than passing on a CRC mismatch they also caused.
    blob.magic ^= 0xFF;
    blob.crc = OpenSkyhawk::calBlobCrc(blob);
    check(F("[F11] wrong magic rejected (CRC still good)"), !OpenSkyhawk::calBlobValid(blob));
    blob.magic ^= 0xFF;

    blob.version = OpenSkyhawk::CAL_VERSION + 1;
    blob.crc = OpenSkyhawk::calBlobCrc(blob);
    check(F("[F12] wrong version rejected (CRC still good)"), !OpenSkyhawk::calBlobValid(blob));
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println(F("=== SimGateway axis calibration transform test ==="));
    testCentreAndRails();
    testTruncation();
    testSymmetricIsIdentity();
    testDegenerate();
    testValidity();
    testBlob();
    Serial.println(g_allPass ? F("=== ALL PASS ===") : F("=== FAILURES PRESENT ==="));
}

void loop() {}
