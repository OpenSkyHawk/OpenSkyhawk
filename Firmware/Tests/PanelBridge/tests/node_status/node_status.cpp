// PanelBridge — node_status test (#86)
//
// Purpose: verify node-status reporting. PanelBridge surfaces connected PanelGroup
// nodes + health to the host as `_OSH_NODE` DCS-BIOS command messages, emitted on:
//   - node alive (dead/unseen → alive transition),
//   - host request (full roster), and
//   - node removal (heartbeat timeout, present=00, cached health retained).
// Exercises the PANELBRIDGE_NODE_STATUS path via PANELBRIDGE_TEST hooks — no CAN
// bus or second board required (heartbeats are injected directly).
//
// Hardware: single STM32F103CB board.
//   - Serial (UART2 PA2/PA3) → USB-UART adapter → PC terminal @ 250000 baud
//   - DiagSerial (USART1 PA9/PA10) @ 115200 baud (phase log)
//   - STLink for flash
//
// HOW TO USE:
//   1. pio run -e test_node_status -t upload
//   2. Open USB-UART @ 250000 (the _OSH_NODE messages) and DiagSerial @ 115200 (phase log).
//
// Expected on USB-UART (250000), repeating each cycle:
//   Phase A — two nodes go alive (transition emit):
//     _OSH_NODE 0101000A00120000
//     _OSH_NODE 0201001400340002
//   Phase B — host request (full roster):
//     _OSH_NODE 0101000A00120000
//     _OSH_NODE 0201001400340002
//   Phase C — heartbeat timeout (~3 s after Phase A) removes both (present=00):
//     _OSH_NODE 0100000A00120000
//     _OSH_NODE 0200001400340002
//
// hex = nodeId(2) present(2) flags(2) uptime(4) rxCount(4) esr(4), big-endian nibbles.

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
#include <STM32Board.h>

static uint8_t  _phase  = 0;
static uint32_t _lastMs = 0;

void setup() {
    STM32Board::diagSerial().begin(115200);
    STM32Board::diagSerial().println(F("=== PanelBridge: node_status (#86) ==="));
    STM32Board::setDebug(true);
    PanelBridge::setup();   // starts Serial @ 250000 + CAN
    DcsBios::setup();
    STM32Board::diagSerial().println(F("[TEST] watch USB-UART (250000) for _OSH_NODE lines"));
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();    // real timeout sweep removes nodes ~3 s after the last heartbeat

    uint32_t now = millis();
    if (now - _lastMs < 2500) return;
    _lastMs = now;
    auto& d = STM32Board::diagSerial();

    switch (_phase) {
    case 0:  // Phase A — inject heartbeats; both nodes transition to alive
        d.println(F("[TEST] A: feed HB node 1 & 2 -> expect 2x _OSH_NODE ...01..."));
        PanelBridge::testFeedHeartbeat(1, 0x00, 10, 18, 0x0000);
        PanelBridge::testFeedHeartbeat(2, 0x02, 20, 52, 0x0002);
        _phase = 1;
        break;
    case 1:  // Phase B — host roster request
        d.println(F("[TEST] B: request -> expect 2x _OSH_NODE roster"));
        PanelBridge::testRequestNodeStatus();
        d.println(F("[TEST] C: stop feeding -> ~3 s timeout removes both (present=00)"));
        _phase = 2;
        break;
    default: // idle a cycle so the real 3 s timeout fires, then repeat
        d.println(F("[TEST] --- cycle ---"));
        _phase = 0;
        break;
    }
}
