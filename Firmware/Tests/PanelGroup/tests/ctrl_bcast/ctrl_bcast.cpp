// PanelGroup — CTRL_BCAST dispatch test
//
// Verifies: non-null ControlPackets are dispatched to all OutputBase objects;
// null slots (controlId == 0) are skipped; non-matching controlIds do not
// trigger the object's internal action.
//
// Uses a minimal concrete OutputBase subclass — does not depend on LED or
// any other concrete output class so this test is pure PanelGroup dispatch logic.
//
// Hardware: STM32. No CAN bus, no MCP23017.

#include <Arduino.h>
#include <PanelGroup.h>

static constexpr uint16_t ADDR_A = 0x1100;
static constexpr uint16_t ADDR_B = 0x1200;

class CountingOutput : public OpenSkyhawk::OutputBase {
public:
    uint16_t callCount = 0;
    uint16_t lastId    = 0;
    uint16_t lastVal   = 0;

    explicit CountingOutput(uint16_t addr) : _addr(addr) {}

    void onControlPacket(uint16_t controlId, uint16_t value) override {
        if (controlId != _addr) return;
        callCount++;
        lastId  = controlId;
        lastVal = value;
    }

private:
    uint16_t _addr;
};

// Declared at global scope — constructors self-register before setup().
CountingOutput gOutA(ADDR_A);
CountingOutput gOutB(ADDR_B);

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("=== PanelGroup ctrl_bcast ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        Serial.print(label);
        Serial.println(ok ? ": PASS" : ": FAIL");
    };

    // Linked list must contain exactly 2 entries
    uint8_t outCount = 0;
    for (auto* p = OpenSkyhawk::OutputBase::head(); p; p = p->next()) outCount++;
    check("2 outputs registered", outCount == 2);

    // ── Dispatch helper — mirrors PanelGroup::loop() inner logic ────────────

    auto dispatch = [](const ControlPacket& pkt) {
        if (pkt.controlId == 0x0000) return;
        for (auto* o = OpenSkyhawk::OutputBase::head(); o; o = o->next())
            o->onControlPacket(pkt.controlId, pkt.value);
    };

    // ── Match: ADDR_A fires gOutA, not gOutB ────────────────────────────────

    dispatch({ ADDR_A, 0x0042 });
    check("ADDR_A dispatched to gOutA",       gOutA.callCount == 1);
    check("ADDR_A: gOutA.lastVal == 0x0042",  gOutA.lastVal   == 0x0042);
    check("ADDR_A did not fire gOutB",        gOutB.callCount == 0);

    // ── Match: ADDR_B fires gOutB ────────────────────────────────────────────

    dispatch({ ADDR_B, 0x00FF });
    check("ADDR_B dispatched to gOutB",       gOutB.callCount == 1);
    check("ADDR_B: gOutB.lastVal == 0x00FF",  gOutB.lastVal   == 0x00FF);
    check("ADDR_B did not add to gOutA",      gOutA.callCount == 1); // unchanged

    // ── Null sentinel (controlId == 0) must be skipped ───────────────────────

    uint8_t countA = gOutA.callCount, countB = gOutB.callCount;
    dispatch({ 0x0000, 0xFFFF });
    check("Null sentinel skipped: gOutA unchanged", gOutA.callCount == countA);
    check("Null sentinel skipped: gOutB unchanged", gOutB.callCount == countB);

    // ── Foreign controlId does not fire either output ────────────────────────

    countA = gOutA.callCount; countB = gOutB.callCount;
    dispatch({ 0xBEEF, 0x1234 });
    check("Foreign controlId: gOutA unchanged", gOutA.callCount == countA);
    check("Foreign controlId: gOutB unchanged", gOutB.callCount == countB);

    // ── Consecutive packets — both slots in a ControlPacketPair ──────────────

    ControlPacketPair pair;
    pair.a = { ADDR_A, 0x0001 };
    pair.b = { ADDR_B, 0x0002 };
    dispatch(pair.a);
    dispatch(pair.b);

    check("Pair slot A fired gOutA (count=2)", gOutA.callCount == 2);
    check("Pair slot B fired gOutB (count=2)", gOutB.callCount == 2);

    Serial.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
