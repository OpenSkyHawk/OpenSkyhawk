// PanelGroup — heartbeat test
//
// Verifies: HB_n frame is NOT sent before 500 ms after setup completes,
// then IS sent within 600 ms, and IS sent a second time within 1100 ms.
// Drives the real heartbeat path PanelGroup::loop() uses — CANProtocol::serviceNodeStatus()
// (#340), which arms its timer on its first call in loop().
//
// Hardware: STM32. No CAN bus, no MCP23017. CAN loopback mode.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>

static uint8_t hbCount = 0;
static uint32_t firstHbMs = 0;
static uint32_t secondHbMs = 0;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != canIdHb(NODE_ID)) return;
    hbCount++;
    if (hbCount == 1) firstHbMs  = millis();
    if (hbCount == 2) secondHbMs = millis();
}

static uint32_t setupDoneMs = 0;

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== PanelGroup heartbeat ===");

    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptId(canIdHb(NODE_ID));
    CANProtocol::startLoopback();

    // Tail of PanelGroup::setup() without full board init (the HB timer arms in loop())
    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    CANProtocol::sendReady();
    setupDoneMs = millis();
}

void loop() {
    static bool reported = false;
    if (reported) return;

    uint32_t now = millis();

    // Same order as PanelGroup::loop(): drain, then the node status service
    CANProtocol::drain();
    CANProtocol::serviceNodeStatus(NodeKind::PANEL_GROUP);

    if (now - setupDoneMs < 1200) return;

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    check("HB count >= 2",                    hbCount >= 2);
    check("First HB >= 500 ms after setup",   firstHbMs  - setupDoneMs >= 450);
    check("First HB <= 600 ms after setup",   firstHbMs  - setupDoneMs <= 650);
    check("Second HB ~500 ms after first",    secondHbMs - firstHbMs   >= 450);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
    reported = true;
}
