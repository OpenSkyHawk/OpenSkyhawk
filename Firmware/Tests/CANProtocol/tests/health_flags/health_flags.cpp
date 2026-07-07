// CANProtocol — HEALTH_n flag coexistence test (#163 / #213)
//
// Built with -DNODE_OVERHEAT_C=0 so any non-sentinel dieTempC trips the OVERHEAT bit. Passing a
// non-NONE fault must ALSO set DEGRADED — proving makeNodeHealthPayload's `|=` never clobbers the
// overheat bit (both bits coexist). A pure-logic check; no hardware needed.
//
// Pass: every line [PASS]; prints "=== ALL PASS ===".

#include <STM32Board.h>
#include <CANProtocol.h>

static bool pass = true;
static void check(const char* label, bool ok) {
    if (!ok) pass = false;
    auto& d = STM32Board::diagSerial();
    d.print(ok ? F("[PASS] ") : F("[FAIL] "));
    d.println(label);
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== health_flags (overheat + degraded, NODE_OVERHEAT_C=0) ==="));

    const uint8_t OVERHEAT = (uint8_t)NodeHealthFlag::OVERHEAT;   // 0x01
    const uint8_t DEGRADED = (uint8_t)NodeHealthFlag::DEGRADED;   // 0x02

    // Hot + healthy: overheat only.
    NodeHealthPayload hot = CANProtocol::makeNodeHealthPayload(NODE_ID, /*dieTempC*/ 40);
    check("hot healthy -> OVERHEAT only", hot.flags == OVERHEAT);

    // Hot + faulted: both bits, faultId carries the code.
    NodeHealthPayload both = CANProtocol::makeNodeHealthPayload(
        NODE_ID, /*dieTempC*/ 40, NodeFaultCode::I2C_PERIPHERAL);
    check("hot + fault -> OVERHEAT|DEGRADED", both.flags == (OVERHEAT | DEGRADED));
    check("hot + fault -> faultId set",       both.faultId == (uint8_t)NodeFaultCode::I2C_PERIPHERAL);

    // Sentinel temp (unavailable) never trips overheat; fault alone -> DEGRADED only.
    NodeHealthPayload deg = CANProtocol::makeNodeHealthPayload(
        NODE_ID, INT8_MIN, NodeFaultCode::OVER_VOLTAGE);
    check("no-temp + fault -> DEGRADED only", deg.flags == DEGRADED);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {}
