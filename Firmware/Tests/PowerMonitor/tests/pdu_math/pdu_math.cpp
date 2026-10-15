// PowerMonitor — PduMath conversions (pure logic, injected values).
//
// Expected values worked by hand from the Rev 0.2.0 front end: 12 V divider 43/10, 5 V divider
// 23/13, INA180A2 ×50 on 10 mΩ (2 mA per mV), NTC 10k B3450 under a 10k bias. Ends with
// "=== pdu_math ALL PASS (n) ===".

#include "../common/check.h"
#include <PduMath.h>
#include <PduConfig.h>
#include <string.h>

using namespace OpenSkyhawk::Pdu;

void setup() {
    begin("pdu_math");

    // VDDA: nominal until Vrefint is anchored, then Vrefint-tracked.
    checkEq("vdda uncalibrated -> nominal 3300", vddaMv(23831, 0), 3300);
    checkEq("vdda calibrated 1200 mV @ raw 23831", vddaMv(23831, 1200), 3300);
    checkEq("vdda calibrated, droop raw 24550", vddaMv(24550, 1200), 3203);
    checkEq("vdda unreadable reference -> nominal", vddaMv(0, 1200), 3300);

    checkEq("counts 32768 @ 3300 -> 1650 mV", countsToMv(32768, 3300), 1650);
    checkEq("12 V rail: 2790 mV pin -> 11997 mV", rail12Mv(2790, 10000), 11997);
    checkEq("5 V rail: 2826 mV pin -> 4999 mV", rail5Mv(2826, 10000), 4999);
    checkEq("12 V rail gain trim x1.01", rail12Mv(2790, 10100), 12116);

    checkEq("current 1500 mV -> 3000 mA", currentMa(1500, 0, 10000), 3000);
    checkEq("current with 50 mA zero -> 2950", currentMa(1500, 500, 10000), 2950);
    checkEq("current zero + gain x1.01 -> 2980", currentMa(1500, 500, 10100), 2980);
    checkEq("residual offset stays negative", currentMa(10, 500, 10000), -30);
    checkEq("raw current 25 mV -> 500 dmA", rawCurrentDmA(25), 500);

    bool fault = true;
    checkEq("NTC mid-scale -> 25.0 C", ntcTempDC(32768, 0, fault), 250);
    check("NTC mid-scale not a fault", !fault);
    checkNear("NTC 4085 ohm -> 50.0 C", ntcTempDC(19007, 0, fault), 500, 2);
    checkNear("NTC 28835 ohm -> 0.0 C", ntcTempDC(48661, 0, fault), 0, 2);
    checkEq("NTC offset applied", ntcTempDC(32768, -15, fault), 235);
    checkEq("NTC shorted -> unavailable", ntcTempDC(500, 0, fault), INT16_MIN);
    check("NTC shorted flagged", fault);
    ntcTempDC(65000, 0, fault);
    check("NTC open flagged", fault);

    check("railed at 65300", railed(65300));
    check("not railed at 60000", !railed(60000));

    const char* v = "123456789";
    checkEq("crc32 check value", (long)crc32(reinterpret_cast<const uint8_t*>(v), strlen(v)), (long)0xCBF43926u);
    checkEq("clampI16 high", clampI16(40000), 32767);
    checkEq("clampU16 negative", clampU16(-5), 0);

    summary("pdu_math");
}

void loop() {}
