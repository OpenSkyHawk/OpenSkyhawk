// PanelGroup — overheat-node bench (#213 / SkyHawkClient#40)
//
// A real PanelGroup node (NODE_ID=1) with NO inputs/outputs, built with NODE_OVERHEAT_C forced
// WELL BELOW the F103 idle die temperature (~30-37 C). Every HEALTH_1 TX therefore sets the
// OVERHEAT flag (bit0) — with no FaultSource on the board there's no DEGRADED, so this exercises
// the overheat render IN ISOLATION: the client shows the amber dot + a 🔥 gold temperature and
// NO fault badge. (The threshold is a deliberate bench value, not a real trip point — the real
// sensor is uncalibrated and needs field data before a sane NODE_OVERHEAT_C is chosen.)
//
// Pair with a PanelBridge node on the CAN bus (NODE_ID=0), same rig as degraded_node.
// Flash: ~/.platformio/penv/bin/pio run -d Firmware/Tests/PanelGroup -e overheat_node -t upload

#include <STM32Board.h>
#include <PanelGroup.h>

void setup() {
    STM32Board::setDebug(true);
    PanelGroup::setup();   // STM32Board::begin() + CAN; no I/O registered — HEALTH_1 still TXes every 1 s
}

void loop() {
    PanelGroup::loop();    // HEALTH_1: dieTempC >= NODE_OVERHEAT_C -> OVERHEAT flag set
}
