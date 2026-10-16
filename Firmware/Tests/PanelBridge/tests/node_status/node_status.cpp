// PanelBridge — node_status test (#86, v3 #344)
//
// Purpose: verify node-status reporting. PanelBridge surfaces connected nodes + health to the
// host as `_NODE_STATUS` DCS-BIOS command messages (proto v3, 28 hex), emitted on:
//   - node alive (dead/unseen → alive transition),
//   - a node's board kind changing (its first HEALTH_n after the alive delta) (#344),
//   - host request (full roster: the bridge's own node-0 row first), and
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
// Also at boot (before the bridge's first HEALTH_0, so its row reads not-yet-seen 80 / kind 00):
//   _NODE_STATUS 0001ffuuuu0000eeee8000000000
//   _NODE_STATUS_END 1
//
// Expected on USB-UART (250000), repeating each cycle. Node 1 is fed a DEGRADED HEALTH frame
// before it goes alive (42 °C → dieTempC 2A; hFlags 02 DEGRADED; faultMask 00 reserved;
// faultId 01 I2C_PERIPHERAL; nodeKind 01 PANEL_GROUP) — this proves the #163 degraded fields and
// the #344 kind forward through the cache into _NODE_STATUS. Node 2 goes alive with no HEALTH
// (temp 80, kind 00 — not yet seen), then gets its first HEALTH (35 °C, kind 02 PDU), which pushes
// one delta:
//   Phase A — two nodes go alive (bare delta emits, no terminator):
//     _NODE_STATUS 010100000A001200002A02000101
//     _NODE_STATUS 0201020014003400028000000000
//   Phase B — node 2's first HEALTH (kind 00 → 02 delta), then host request (full roster,
//   node-0 row first, terminated; count includes node 0):
//     _NODE_STATUS 0201020014003400022300000002
//     _NODE_STATUS 0001ffuuuu0000eeeett00000003
//     _NODE_STATUS 010100000A001200002A02000101
//     _NODE_STATUS 0201020014003400022300000002
//     _NODE_STATUS_END 3
//   Phase C — heartbeat timeout (~3 s after Phase A) removes both (present=00 deltas,
//   cached health retained, incl. node 1's degraded fields and both kinds):
//     _NODE_STATUS 010000000A001200002A02000101
//     _NODE_STATUS 0200020014003400022300000002
//
// hex = nodeId(2) present(2) flags(2) uptime(4) rxCount(4) esr(4) dieTempC(2) hFlags(2)
//       faultMask(2) faultId(2) nodeKind(2); each field a fixed-width hex number (MSN first).
// dieTempC int8 two's-complement (80 = unseen). Node-0 row: ff = the bridge's own CAN flags,
// uuuu = its uptime in s, eeee = its ESR, tt = its die temp, kind 03 BRIDGE — live values.

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
#include <STM32Board.h>
#include <NodeStatus.h>  // NodeHealthFlag / NodeFaultCode / NodeKind for the HEALTH injections (#163/#344)

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
        d.println(F("[TEST] A: feed HB node 1 & 2 + DEGRADED HEALTH node 1 -> expect 2x _NODE_STATUS ...01..."));
        // Node 2 back to not-yet-seen (it is dead here, so no delta), so every cycle repeats.
        PanelBridge::testFeedHealth(2, INT8_MIN);
        // dieTempC 2A + DEGRADED/I2C_PERIPHERAL + PANEL_GROUP — cache before the alive edge emits
        PanelBridge::testFeedHealth(1, 42,
                                    (uint8_t)NodeHealthFlag::DEGRADED, 0,
                                    (uint8_t)NodeFaultCode::I2C_PERIPHERAL,
                                    (uint8_t)NodeKind::PANEL_GROUP);
        PanelBridge::testFeedHeartbeat(1, 0x00, 10, 18, 0x0000);
        PanelBridge::testFeedHeartbeat(2, 0x02, 20, 52, 0x0002);  // node 2: no HEALTH → temp 80
        _phase = 1;
        break;
    case 1:  // Phase B — host roster request
        d.println(F("[TEST] B: node 2 first HEALTH (kind PDU) -> expect 1 delta"));
        PanelBridge::testFeedHealth(2, 35, 0, 0, 0, (uint8_t)NodeKind::PDU);
        d.println(F("[TEST] B: request -> expect node-0 row + 2x _NODE_STATUS + _NODE_STATUS_END 3"));
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
