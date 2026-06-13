// PanelBridge — STM32F103CBT6
//
// CAN master node. Runs the DCS-BIOS library on UART2 (Serial, PA2/PA3 → SimGateway).
// Broadcasts all DCS output values over CAN (CTRL_BCAST).
// Routes CAN EVTs from PanelGroup nodes:
//   controlId 0x8001–0x86FF → binary search in A4EC_InputMap → sendDcsBiosMessage()
//   controlId < 0x8000      → HID frame (0xAA 0x55 ...) → UART → SimGateway
//
// No per-panel control declarations needed — A4EC_InputMap covers the full A-4E-C set.
// NODE_ID=0 is required; PanelBridge is the CAN master and does not transmit HB_0.

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
#include <STM32Board.h>

void onNodeAlive(uint8_t nodeId) {
    (void)nodeId;  // add diagnostics here if needed
}

void onNodeDead(uint8_t nodeId) {
    (void)nodeId;  // add diagnostics here if needed
}

void setup() {
    STM32Board::setDebug(true);       // remove for production; enables DiagSerial output
    PanelBridge::onNodeAlive(onNodeAlive);
    PanelBridge::onNodeDead(onNodeDead);
    PanelBridge::setup();
    DcsBios::setup();
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();
}
