// PanelBridge — node_status test (#86)
//
// Purpose: verify node-status reporting. PanelBridge surfaces connected PanelGroup
// nodes + health to the host as `_NODE_STATUS` DCS-BIOS command messages, emitted on:
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
//   2. Open USB-UART @ 250000 (the _NODE_STATUS messages) and DiagSerial @ 115200 (phase log).
//
// Also at boot: _NODE_STATUS_END 0 (empty roster seed).
//
// Expected on USB-UART (250000), repeating each cycle. Node 1 is also fed a HEALTH
// frame (42 °C → dieTempC 2A); node 2 gets none, so its temp stays at the not-yet-seen
// sentinel 80. The three health fields hFlags/faultMask/faultId are 0 here — the degraded
// feature (#163) isn't populating them yet:
//   Phase A — two nodes go alive (bare delta emits, no terminator):
//     _NODE_STATUS 010100000A001200002A000000
//     _NODE_STATUS 02010200140034000280000000
//   Phase B — host request (full roster, terminated):
//     _NODE_STATUS 010100000A001200002A000000
//     _NODE_STATUS 02010200140034000280000000
//     _NODE_STATUS_END 2
//   Phase C — heartbeat timeout (~3 s after Phase A) removes both (present=00 deltas,
//   cached health retained):
//     _NODE_STATUS 010000000A001200002A000000
//     _NODE_STATUS 02000200140034000280000000
//
// hex = nodeId(2) present(2) flags(2) uptime(4) rxCount(4) esr(4) dieTempC(2) hFlags(2)
//       faultMask(2) faultId(2); each field a fixed-width hex number (MSN first).
// dieTempC int8 two's-complement (80 = unseen).

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
    STM32Board::diagSerial().println(F("[TEST] watch USB-UART (250000) for _NODE_STATUS lines"));
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
        d.println(F("[TEST] A: feed HB node 1 & 2 + HEALTH node 1 -> expect 2x _NODE_STATUS ...01..."));
        PanelBridge::testFeedHealth(1, 42);  // dieTempC 2A — cache before the alive edge emits
        PanelBridge::testFeedHeartbeat(1, 0x00, 10, 18, 0x0000);
        PanelBridge::testFeedHeartbeat(2, 0x02, 20, 52, 0x0002);  // node 2: no HEALTH → temp 80
        _phase = 1;
        break;
    case 1:  // Phase B — host roster request
        d.println(F("[TEST] B: request -> expect 2x _NODE_STATUS roster"));
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
