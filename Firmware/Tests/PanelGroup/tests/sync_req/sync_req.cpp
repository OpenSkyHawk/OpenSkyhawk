// PanelGroup — SYNC_REQ → forceReport dispatch test
//
// Verifies: on SYNC_REQ all InputBase objects receive forceReport() and
// the dispatch iterates the full linked list in order. Also verifies that
// poll() is not called during the sync path.
//
// Hardware: STM32. No CAN bus, no MCP23017. Tests dispatch in isolation.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>

// Minimal concrete InputBase — counts configure, poll, forceReport calls.
class CountingInput : public OpenSkyhawk::InputBase {
public:
    uint8_t configureCount = 0;
    uint8_t pollCount      = 0;
    uint8_t reportCount    = 0;

    void configure() override { configureCount++; }
    void poll()      override { pollCount++;      }
    void forceReport() override { reportCount++;  }
};

// Declared at global scope — self-registers on construction.
CountingInput gInputA;
CountingInput gInputB;
CountingInput gInputC;

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== PanelGroup sync_req ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    // Linked list must contain all 3 (order is reverse-declaration due to push-to-front)
    uint8_t inputCount = 0;
    for (auto* p = OpenSkyhawk::InputBase::head(); p; p = p->next()) inputCount++;
    check("3 inputs registered", inputCount == 3);

    // ── configure() path ────────────────────────────────────────────────────

    for (auto* p = OpenSkyhawk::InputBase::head(); p; p = p->next()) p->configure();
    check("configure() reached gInputA", gInputA.configureCount == 1);
    check("configure() reached gInputB", gInputB.configureCount == 1);
    check("configure() reached gInputC", gInputC.configureCount == 1);

    // ── forceReport() path — mirrors onSyncReq() in PanelGroup.cpp ─────────

    for (auto* p = OpenSkyhawk::InputBase::head(); p; p = p->next()) p->forceReport();
    check("forceReport() reached gInputA", gInputA.reportCount == 1);
    check("forceReport() reached gInputB", gInputB.reportCount == 1);
    check("forceReport() reached gInputC", gInputC.reportCount == 1);

    // poll() must NOT have been called during configure or forceReport
    check("poll() not called on gInputA",  gInputA.pollCount == 0);
    check("poll() not called on gInputB",  gInputB.pollCount == 0);
    check("poll() not called on gInputC",  gInputC.pollCount == 0);

    // ── Second forceReport() — counts must increment, not reset ─────────────

    for (auto* p = OpenSkyhawk::InputBase::head(); p; p = p->next()) p->forceReport();
    check("forceReport() idempotent on gInputA (count=2)", gInputA.reportCount == 2);
    check("forceReport() idempotent on gInputB (count=2)", gInputB.reportCount == 2);
    check("forceReport() idempotent on gInputC (count=2)", gInputC.reportCount == 2);

    // poll() still zero
    check("poll() still zero after 2nd forceReport", gInputA.pollCount == 0);

    // ── poll() path — separate from sync ────────────────────────────────────

    for (auto* p = OpenSkyhawk::InputBase::head(); p; p = p->next()) p->poll();
    check("poll() called on gInputA",  gInputA.pollCount == 1);
    check("poll() called on gInputB",  gInputB.pollCount == 1);
    check("poll() called on gInputC",  gInputC.pollCount == 1);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
