// PanelBridge — startup_sequence test
//
// Purpose: verify setup order, pass-all CAN filter, CAN start, and cold-boot SYNC_REQ.
//
// HOW TO USE:
//   1. Flash:   pio run -e test_startup_sequence -t upload
//   2. Connect ST-Link and open DiagSerial (USART1 PA9/PA10, 115200 baud).
//   3. Power on — within 100 ms you should see:
//        [BRIDGE] SYNC_REQ broadcast
//      If CAN bus is absent (no termination / no second node), the CAN controller
//      may enter BUS_OFF after the first SYNC_REQ transmission attempt.
//      That is expected for a single-board test; the log line still confirms
//      setup() ran and called CANProtocol::start() + broadcastSyncReq().
//
// Pass criteria (manual, from DiagSerial):
//   - "[BRIDGE] SYNC_REQ broadcast" appears on boot
//   - No hard-fault or silence (indicates setup() order is correct)
//
// No physical CAN bus required. STM32F103CB with STLink required.

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
#include <STM32Board.h>

static bool nodeAliveReceived = false;
static bool nodeDeadReceived  = false;

void onNodeAlive(uint8_t nodeId) {
    nodeAliveReceived = true;
    auto& d = STM32Board::diagSerial();
    d.print(F("[TEST] onNodeAlive node=")); d.println(nodeId);
}
void onNodeDead(uint8_t nodeId) {
    nodeDeadReceived = true;
    auto& d = STM32Board::diagSerial();
    d.print(F("[TEST] onNodeDead node=")); d.println(nodeId);
}

void setup() {
    STM32Board::diagSerial().begin(115200);
    STM32Board::diagSerial().println(F("=== PanelBridge: startup_sequence ==="));
    STM32Board::setDebug(true);
    PanelBridge::onNodeAlive(onNodeAlive);
    PanelBridge::onNodeDead(onNodeDead);
    PanelBridge::setup();
    DcsBios::setup();
    STM32Board::diagSerial().println(F("[TEST] startup_sequence — setup complete"));
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();
}
