// PanelBridge — dual_integration test
//
// Purpose: test the full startup/reconnect lifecycle on a physical CAN bus with two boards.
// Validates all three startup timelines from FirmwarePlan/09-startup-resync-diagnostics.md.
//
// Hardware: two STM32F103CB boards on a physical CAN bus (120Ω termination on each end).
//   NODE_ID=0  PanelBridge role — flash with test_dual_integration_bridge
//   NODE_ID=1  PanelGroup role  — flash with test_dual_integration_node
//
// HOW TO USE:
//   1. Flash both boards.
//   2. Connect DiagSerial (USART1 PA9/PA10, 115200 baud) on BOTH boards simultaneously.
//   3. Power both boards. Observe the following event sequence.
//
// ── Timeline A: PanelBridge boots first ──────────────────────────────────────
//   NODE_ID=0: [BRIDGE] SYNC_REQ broadcast    ← cold boot
//   NODE_ID=1: [NODE] SYNC_REQ received, re-polling
//   NODE_ID=1: [NODE] EVT burst (initial state)
//   NODE_ID=1: [NODE] READY sent
//   NODE_ID=0: [BRIDGE] READY node=1
//   NODE_ID=0: [BRIDGE] node=1 alive
//   NODE_ID=0: [BRIDGE] SYNC_REQ broadcast    ← after READY
//   NODE_ID=1: [NODE] SYNC_REQ received, re-polling
//
// ── Timeline B: PanelGroup boots first ───────────────────────────────────────
//   NODE_ID=1: boots, sends EVT burst and READY (lost — bridge not yet listening)
//   NODE_ID=0: [BRIDGE] SYNC_REQ broadcast    ← cold boot
//   NODE_ID=1: [NODE] SYNC_REQ received, re-polling
//   NODE_ID=0: [BRIDGE] node=1 alive (after HB or READY received)
//   → same end state as Timeline A
//
// ── Timeline C: Reconnect after dropout ──────────────────────────────────────
//   Both running → power-cycle NODE_ID=1 board
//   NODE_ID=0: [BRIDGE] node=1 dead           ← after ~3 s no HB
//   NODE_ID=1: (reconnects, sends READY + HBs)
//   NODE_ID=0: [BRIDGE] node=1 alive
//   NODE_ID=0: [BRIDGE] SYNC_REQ broadcast    ← recovery sync
//   NODE_ID=1: [NODE] SYNC_REQ received, re-polling
//
// Pass criteria (manual, both DiagSerials):
//   - All three timelines produce correct alive/dead/SYNC_REQ sequence
//   - "[BRIDGE] SYNC_REQ broadcast" appears at startup AND after each READY/node-alive
//   - "[NODE] SYNC_REQ received, re-polling" appears in response to each SYNC_REQ
//   - "[BRIDGE] node=1 dead" appears if PanelGroup is power-cycled; alive reappears after reconnect

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
    d.print(F("[BRIDGE] node=")); d.print(nodeId); d.println(F(" alive"));
}
void onNodeDead(uint8_t nodeId) {
    auto& d = STM32Board::diagSerial();
    d.print(F("[BRIDGE] node=")); d.print(nodeId); d.println(F(" dead"));
}

void setup() {
    STM32Board::setDebug(true);
    PanelBridge::onNodeAlive(onNodeAlive);
    PanelBridge::onNodeDead(onNodeDead);
    PanelBridge::setup();
    DcsBios::setup();
    STM32Board::diagSerial().println(F("[BRIDGE] dual_integration — ready"));
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();
}

#else
// ── PanelGroup role (simulated) ───────────────────────────────────────────────
// Mimics the PanelGroup boot sequence: EVT burst → READY → periodic HBs.
// Re-polls (sends burst) on each SYNC_REQ.
// TODO Phase 3: replace with PanelGroup::setup()/loop() once PanelGroup library exists.

static uint32_t _lastHbMs  = 0;
static bool     _readySent = false;

// Simulated input state — one toggle-switch EVT for a known controlId
static void sendEvtBurst() {
    ControlPacketPair pair = {};
    pair.a.controlId = 0x8001;  // DCSIN_ACCEL_RESET — representative cockpit input
    pair.a.value     = 0;
    CANProtocol::send(canIdEvt(NODE_ID), (const uint8_t*)&pair, 8);
    STM32Board::diagSerial().println(F("[NODE] EVT burst sent"));
}

static void onSyncReq() {
    STM32Board::diagSerial().println(F("[NODE] SYNC_REQ received, re-polling"));
    sendEvtBurst();
}

void setup() {
    STM32Board::begin();
    STM32Board::setDebug(true);
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::filterAcceptAll();
    CANProtocol::onSyncReq(onSyncReq);
    CANProtocol::start();
    STM32Board::diagSerial().println(F("[NODE] dual_integration — ready"));
}

void loop() {
    uint32_t now = millis();
    STM32Board::tick();
    CANProtocol::drain();

    // Boot sequence: EVT burst then READY after 200 ms
    if (!_readySent && now > 200) {
        sendEvtBurst();
        CANProtocol::send(canIdReady(NODE_ID), nullptr, 0);
        _readySent = true;
        STM32Board::diagSerial().println(F("[NODE] READY sent"));
    }

    // Periodic heartbeat
    if (now - _lastHbMs >= 500) {
        _lastHbMs = now;
        HeartbeatPayload hb = CANProtocol::makeHeartbeatPayload(NODE_ID, 0);
        CANProtocol::send(canIdHb(NODE_ID), (const uint8_t*)&hb, sizeof(hb));
    }
}
#endif
