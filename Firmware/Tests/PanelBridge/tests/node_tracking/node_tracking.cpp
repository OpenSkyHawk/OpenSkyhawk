// PanelBridge — node_tracking test
//
// Purpose: verify READY sync, alive/dead callbacks, heartbeat timeout, and recovery.
//
// Hardware: two STM32F103CB boards on a physical CAN bus (120Ω termination on each end).
//   NODE_ID=0  PanelBridge role — flash with test_node_tracking_bridge
//   NODE_ID=1  PanelGroup role  — flash with test_node_tracking_node
//
// HOW TO USE:
//   1. Flash both boards.
//   2. Connect DiagSerial on NODE_ID=0 (USART1 PA9/PA10, 115200 baud).
//   3. Power both boards. Verify:
//        [BRIDGE] SYNC_REQ broadcast         ← cold boot
//        [BRIDGE] node=1 alive               ← after NODE_ID=1 sends first HB
//        [BRIDGE] SYNC_REQ broadcast         ← recovery broadcast after alive
//   4. Power-cycle the NODE_ID=1 board. After ~3 s, verify:
//        [BRIDGE] node=1 dead
//   5. Power it back on. Verify alive/SYNC_REQ again (Timeline C reconnect).
//
// Pass criteria (manual, DiagSerial on NODE_ID=0):
//   - alive appears after NODE_ID=1 boots
//   - dead appears ~3 s after NODE_ID=1 is unplugged
//   - alive + SYNC_REQ appear again after NODE_ID=1 reconnects

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <Arduino.h>
#include <STM32Board.h>
#include <CANProtocol.h>

#if NODE_ID == 0
// ── PanelBridge role ──────────────────────────────────────────────────────────
#include <PanelBridge.h>

void onNodeAlive(uint8_t nodeId) {
    auto& d = STM32Board::diagSerial();
    d.print(F("[TEST] PASS alive node=")); d.println(nodeId);
}
void onNodeDead(uint8_t nodeId) {
    auto& d = STM32Board::diagSerial();
    d.print(F("[TEST] PASS dead node=")); d.println(nodeId);
}

void setup() {
    STM32Board::diagSerial().begin(115200);
    STM32Board::diagSerial().println(F("=== PanelBridge: node_tracking [bridge] ==="));
    STM32Board::setDebug(true);
    PanelBridge::onNodeAlive(onNodeAlive);
    PanelBridge::onNodeDead(onNodeDead);
    PanelBridge::setup();
    DcsBios::setup();
    STM32Board::diagSerial().println(F("[TEST] node_tracking — bridge ready"));
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();
}

#else
// ── PanelGroup role (simulated) ───────────────────────────────────────────────
// Sends heartbeats at 500 ms and a READY frame on boot.
// TODO Phase 3: replace with PanelGroup::setup()/loop() once PanelGroup library exists.

static uint32_t lastHbMs  = 0;
static bool     readySent = false;

static void sendHb() {
    HeartbeatPayload hb = CANProtocol::makeHeartbeatPayload(NODE_ID, 0);
    CANProtocol::send(canIdHb(NODE_ID), (const uint8_t*)&hb, sizeof(hb));
}

static void sendReady() {
    CANProtocol::send(canIdReady(NODE_ID), nullptr, 0);
}

void setup() {
    STM32Board::diagSerial().begin(115200);
    STM32Board::diagSerial().println(F("=== PanelBridge: node_tracking [node] ==="));
    STM32Board::begin();
    STM32Board::setDebug(true);
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::filterAcceptId(CAN_ID_SYNC_REQ);
    CANProtocol::start();
    STM32Board::diagSerial().println(F("[NODE] node_tracking — PanelGroup role ready"));
}

void loop() {
    uint32_t now = millis();
    STM32Board::tick();
    CANProtocol::drain();

    if (!readySent && now > 200) {
        sendReady();
        readySent = true;
        STM32Board::diagSerial().println(F("[NODE] READY sent"));
    }

    if (now - lastHbMs >= 500) {
        lastHbMs = now;
        sendHb();
    }
}
#endif
