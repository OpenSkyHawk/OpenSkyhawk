// NodeStatus — aggregateFaults() test (#163)
//
// Verifies the node fault aggregator over a couple of STATIC mock FaultSources (no hardware):
//   - all healthy → NONE, detail ""
//   - one faulted → its code + detail
//   - two faulted → the HEAD-MOST (last-constructed) wins — registry order is REVERSE
//     construction order (the intrusive list pushes at head)
//   - a source returning a null faultDetail() → normalized to "" (never null out)
//   - detailOut == nullptr is accepted
//
// Isolation: the FaultSource registry is global/permanent with no reset seam — this sketch owns
// the only two fault sources in the binary, so head()=mockB, next=mockA. Runs on a bare F103C8.
//
// Pass: every line [PASS]; prints "=== ALL PASS ===".

#include <Arduino.h>
#include <string.h>
#include <STM32Board.h>
#include <NodeStatus.h>

using namespace OpenSkyhawk;

// Settable mock fault source.
struct MockFault : FaultSource {
    NodeFaultCode code   = NodeFaultCode::NONE;
    const char*   detail = "";
    NodeFaultCode faultCode() const override   { return code; }
    const char*   faultDetail() const override { return detail; }
};

static MockFault mockA;   // constructed first  → tail
static MockFault mockB;   // constructed second → head (reverse-construction order)

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
    d.println(F("=== NodeStatus aggregateFaults (#163) ==="));

    const char* detail = nullptr;

    // 1) All healthy.
    check("all healthy -> NONE",        aggregateFaults(&detail) == NodeFaultCode::NONE);
    check("all healthy -> detail ''",   strcmp(detail, "") == 0);
    check("null detailOut accepted",    aggregateFaults(nullptr) == NodeFaultCode::NONE);

    // 2) One source faults -> its code + detail.
    mockA.code = NodeFaultCode::OVER_VOLTAGE; mockA.detail = "rail A high";
    check("one fault -> its code",      aggregateFaults(&detail) == NodeFaultCode::OVER_VOLTAGE);
    check("one fault -> its detail",    strcmp(detail, "rail A high") == 0);

    // 3) Two sources fault -> the head-most (mockB, last-constructed) wins.
    mockB.code = NodeFaultCode::I2C_PERIPHERAL; mockB.detail = "oled gone";
    check("two faults -> head-most",    aggregateFaults(&detail) == NodeFaultCode::I2C_PERIPHERAL);
    check("head-most detail",           strcmp(detail, "oled gone") == 0);

    // 4) Faulted source with a null faultDetail() -> aggregator normalizes to "".
    mockA.code = NodeFaultCode::NONE;
    mockB.detail = nullptr;
    aggregateFaults(&detail);
    check("null faultDetail -> ''",     strcmp(detail, "") == 0);

    // 5) Back to healthy clears.
    mockB.code = NodeFaultCode::NONE;
    check("recovered -> NONE",          aggregateFaults(&detail) == NodeFaultCode::NONE);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {}
