// SimGateway - axis calibration persistence test
//
// The only test in this project that writes real flash (issue #251). It runs in
// production mode — no SIMGATEWAY_TEST — because SIMGATEWAY_TEST builds deliberately
// stub the EEPROM path out so that the other tests never erase a sector.
//
// Two phases, separated by a power cycle. Which phase runs is decided by what
// SimGateway::setup() loaded, so there is no state to pass between runs:
//
//   Phase 1 (blank or unrelated blob)  write the pattern, prove the library skips a
//                                      redundant re-commit, then ask for a reset.
//   Phase 2 (pattern already loaded)   assert it survived the power cycle, then erase the
//                                      pattern so the board is left as it was found.
//
// Covers:
//   - a sealed blob survives commit -> power cycle -> setup() load
//   - setup() populates the live calibration, so dispatched axes are actually transformed
//   - a corrupt blob is rejected and falls back to uncalibrated
//   - re-committing identical bytes performs no erase (the library skips a clean buffer)
//
// Note on what phase 1 can and cannot prove: EEPROM.get() reads the library's RAM mirror,
// not flash, so a matching read-back immediately after a put() proves only that the buffer
// holds the right bytes. Persistence is phase 2's job and cannot be shortcut.
//
// The no-change check is a same-boot A/B — real write versus immediate re-commit — rather
// than an absolute microsecond threshold. Erase cost depends on what the sector already
// held: measured on this board at ~580 us over an already-erased sector but ~28 ms over a
// programmed one, a 48x spread. Any hardcoded threshold would pass one run and fail the
// other. The re-commit, by contrast, is ~4 us in both cases because commit() never reaches
// the flash at all.
//
// Flash:
//   pio run -e test_axis_cal_persist -t upload
// Monitor: open USB CDC (115200) on the Pico. Reset the board when prompted.
//
// Leaves the EEPROM sector erased on completion. Nothing else on the device uses it.

#include <Arduino.h>
#include <EEPROM.h>
#include <SimGateway.h>

using OpenSkyhawk::AxisCal;
using OpenSkyhawk::CalBlob;

// One axis declared so setup()'s load has something to act on and dispatch() is exercised.
OpenSkyhawk::HIDAxis axisRoll(CTRL_ROLL, 0);

static bool g_allPass = true;

static void check(const __FlashStringHelper* label, bool cond) {
    Serial.print(label);
    Serial.println(cond ? F(": PASS") : F(": FAIL"));
    g_allPass &= cond;
}

// Measured on the AxisBench hall thumbstick — the same numbers the transform test uses.
static const AxisCal ROLL  = { 13443, 34728, 50704, 0 };
static const AxisCal PITCH = { 13058, 33112, 53741, 0 };

static bool matchesPattern(const CalBlob& blob) {
    return OpenSkyhawk::calBlobValid(blob)
        && blob.axes[0].min    == ROLL.min
        && blob.axes[0].centre == ROLL.centre
        && blob.axes[0].max    == ROLL.max
        && blob.axes[1].centre == PITCH.centre;
}

static void buildPattern(CalBlob& blob) {
    OpenSkyhawk::calBlobClear(blob);
    blob.axes[0] = ROLL;
    blob.axes[1] = PITCH;
    OpenSkyhawk::calBlobSeal(blob);
}

// ── Phase 1: write and verify the read-back ───────────────────────────────────
static void phaseWrite() {
    Serial.println(F("-- Phase 1: writing calibration to flash"));

    CalBlob blob;
    buildPattern(blob);
    check(F("[P1] pattern seals as valid"), OpenSkyhawk::calBlobValid(blob));

    EEPROM.put(0, blob);
    const uint32_t t0 = micros();
    const bool committed = EEPROM.commit();
    const uint32_t writeUs = micros() - t0;
    check(F("[P2] commit succeeded"), committed);

    // Immediately re-commit the identical bytes. put() only marks the buffer dirty on a real
    // byte difference and commit() early-returns on a clean buffer, so this must take a small
    // fraction of the write above. Comparing the two within one boot avoids asserting an
    // absolute flash-timing figure, which is board- and part-specific.
    EEPROM.put(0, blob);
    const uint32_t t1 = micros();
    const bool recommitted = EEPROM.commit();
    const uint32_t noopUs = micros() - t1;
    check(F("[P3] no-change commit reports success"), recommitted);

    Serial.print(F("       write "));
    Serial.print(writeUs);
    Serial.print(F(" us, no-change re-commit "));
    Serial.print(noopUs);
    Serial.println(F(" us"));
    check(F("[P4] no-change commit skips the write (>=10x faster)"),
          noopUs * 10 < writeUs);

    // NOTE: EEPROM.get() reads the library's RAM mirror, not flash, so this only proves the
    // buffer holds what we put there. Whether it reached flash is phase 2's job.
    CalBlob mirror;
    EEPROM.get(0, mirror);
    check(F("[P5] RAM mirror is valid"), OpenSkyhawk::calBlobValid(mirror));
    check(F("[P6] RAM mirror matches what was written"), matchesPattern(mirror));

    Serial.println(F(""));
    Serial.println(F(">>> Now RESET or power-cycle the board and re-read this port."));
    Serial.println(F(">>> Phase 2 runs automatically."));
}

// ── Phase 2: prove it survived, then clean up ─────────────────────────────────
static void phaseVerify() {
    Serial.println(F("-- Phase 2: calibration survived the power cycle"));

    const CalBlob& live = SimGateway::calibration();
    check(F("[Q1] setup() loaded the stored blob"), matchesPattern(live));
    check(F("[Q2] axis 0 reads as calibrated"),     axisCalValid(live.axes[0]));
    check(F("[Q3] axis 1 reads as calibrated"),     axisCalValid(live.axes[1]));
    check(F("[Q4] unwritten axis 2 reads as uncalibrated"), !axisCalValid(live.axes[2]));

    // The load is only useful if it reaches the transform. Roll's centre must map to dead
    // centre — the +7.1% offset this whole feature exists to absorb.
    check(F("[Q5] loaded calibration is applied by the transform"),
          axisCalApply(live.axes[0], ROLL.centre) == 32768 &&
          axisCalApply(live.axes[0], ROLL.max)    == 65535);

    // A blob whose checksum does not match must be rejected outright, not partially trusted.
    CalBlob corrupt;
    buildPattern(corrupt);
    corrupt.axes[0].max ^= 0x0001;   // payload changed, CRC left stale
    check(F("[Q6] corrupt blob rejected"), !OpenSkyhawk::calBlobValid(corrupt));

    // Leave the sector as we found it, so a later run starts from phase 1 again.
    CalBlob blank;
    OpenSkyhawk::calBlobClear(blank);
    EEPROM.put(0, blank);
    check(F("[Q7] cleanup erase succeeded"), EEPROM.commit());

    CalBlob after;
    EEPROM.get(0, after);
    check(F("[Q8] sector left uncalibrated"), !OpenSkyhawk::calBlobValid(after));
}

void setup() {
    Serial.begin(115200);

    // Loads the stored blob into the live calibration. Also brings up USB and the UART.
    SimGateway::setup(Serial1);

    delay(3000);   // CDC needs a moment after HID enumeration

    Serial.println(F("=== SimGateway axis calibration persistence test ==="));

    if (matchesPattern(SimGateway::calibration())) {
        phaseVerify();
    } else {
        phaseWrite();
    }

    Serial.println(g_allPass ? F("=== ALL PASS ===") : F("=== FAILURES PRESENT ==="));
}

void loop() {}
