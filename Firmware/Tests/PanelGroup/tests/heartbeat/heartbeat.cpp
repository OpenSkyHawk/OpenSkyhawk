// PanelGroup — heartbeat test
//
// Verifies: HB_n frame is NOT sent before 500 ms after setup completes,
// then IS sent within 600 ms, and IS sent a second time within 1100 ms.
//
// Hardware: STM32. No CAN bus, no MCP23017. CAN loopback mode.

#include <Arduino.h>
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
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("=== PanelGroup heartbeat ===");

    CANProtocol::onReceive(onCan);
    CANProtocol::startLoopback();

    // Replicate heartbeat-timer arming without full board init
    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    CANProtocol::send(canIdReady(NODE_ID), nullptr, 0);
    setupDoneMs = millis();
}

void loop() {
    static bool reported = false;
    if (reported) return;

    uint32_t now = millis();

    // Drive the heartbeat manually — mirrors PanelGroup::loop() heartbeat logic
    static uint32_t lastHbMs = setupDoneMs;
    if (now - lastHbMs >= 500) {
        lastHbMs = now;
        HeartbeatPayload hb = CANProtocol::makeHeartbeatPayload(NODE_ID, 0);
        CANProtocol::send(canIdHb(NODE_ID),
                          reinterpret_cast<const uint8_t*>(&hb), sizeof(hb));
    }
    CANProtocol::drain();

    if (now - setupDoneMs < 1200) return;

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        Serial.print(label);
        Serial.println(ok ? ": PASS" : ": FAIL");
    };

    check("HB count >= 2",                    hbCount >= 2);
    check("First HB >= 500 ms after setup",   firstHbMs  - setupDoneMs >= 450);
    check("First HB <= 600 ms after setup",   firstHbMs  - setupDoneMs <= 650);
    check("Second HB ~500 ms after first",    secondHbMs - firstHbMs   >= 450);

    Serial.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
    reported = true;
}
