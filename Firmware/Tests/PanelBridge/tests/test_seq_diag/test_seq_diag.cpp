// PanelBridge — test_seq_diag test
//
// Purpose: verify that receiving 'T' on DiagSerial triggers a TEST_SEQ CAN frame,
// and that ECHO responses from a PanelGroup node are consumed and RTT is logged.
//
// Hardware: two STM32F103CB boards on a physical CAN bus.
//   NODE_ID=0  PanelBridge role — flash with test_seq_diag_bridge
//   NODE_ID=1  PanelGroup role  — flash with test_seq_diag_node
//
// HOW TO USE:
//   1. Flash both boards.
//   2. Connect DiagSerial on NODE_ID=0 (USART1 PA9/PA10, 115200 baud).
//   3. In terminal, send the character 'T' to DiagSerial.
//   4. Observe on DiagSerial:
//        [BRIDGE] TEST_SEQ seq=1
//        [BRIDGE] ECHO node=1 seq=1 rtt=Xms
//   5. Repeat 'T' to increment sequence number.
//
// Pass criteria (manual, DiagSerial on NODE_ID=0):
//   - TEST_SEQ seq increments on each 'T'
//   - ECHO response received and RTT < 10 ms (typical CAN round-trip)

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <Arduino.h>
#include <STM32Board.h>
#include <CANProtocol.h>

#if NODE_ID == 0
// ── PanelBridge role ──────────────────────────────────────────────────────────
#include <PanelBridge.h>

void setup() {
    STM32Board::diagSerial().begin(115200);
    STM32Board::diagSerial().println(F("=== PanelBridge: test_seq_diag [bridge] ==="));
    STM32Board::setDebug(true);
    PanelBridge::setup();
    DcsBios::setup();
    STM32Board::diagSerial().println(F("[TEST] test_seq_diag — bridge ready"));
    STM32Board::diagSerial().println(F("[TEST] send 'T' on this terminal to trigger TEST_SEQ"));
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();
}

#else
// ── PanelGroup role ───────────────────────────────────────────────────────────
// TEST_SEQ echo is handled automatically by CANProtocol::drain() — not forwarded
// to onReceive(). No manual echo needed here; node just needs to be on the bus
// with HBs running.
// TODO Phase 3: replace with PanelGroup::setup()/loop() once PanelGroup library exists.

static uint32_t _lastHbMs = 0;
static bool     _readySent = false;

void setup() {
    STM32Board::diagSerial().begin(115200);
    STM32Board::diagSerial().println(F("=== PanelBridge: test_seq_diag [node] ==="));
    STM32Board::begin();
    STM32Board::setDebug(true);
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::filterAcceptAll();
    CANProtocol::start();
    STM32Board::diagSerial().println(F("[TEST] test_seq_diag — node ready, auto-echoing TEST_SEQ"));
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    uint32_t now = millis();
    if (!_readySent && now > 200) {
        CANProtocol::send(canIdReady(NODE_ID), nullptr, 0);
        _readySent = true;
    }
    if (now - _lastHbMs >= 500) {
        _lastHbMs = now;
        HeartbeatPayload hb = CANProtocol::makeHeartbeatPayload(NODE_ID, 0);
        CANProtocol::send(canIdHb(NODE_ID), (const uint8_t*)&hb, sizeof(hb));
    }
}
#endif
