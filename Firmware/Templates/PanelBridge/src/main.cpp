// PanelBridge — STM32F103CBT6
//
// CAN master node. Runs the DCS-BIOS library on UART2 (Serial, PA2/PA3 → SimGateway).
// Broadcasts all DCS output values over CAN (CTRL_BCAST).
// Routes CAN EVTs from PanelGroup nodes:
//   controlId 0x8001–0x86FF → binary search in A4EC_InputMap → sendDcsBiosMessage()
//   controlId < 0x8000      → HID frame (0xAA 0x55 ...) → UART → SimGateway
//
// No per-panel control declarations needed — A4EC_InputMap covers the full A-4E-C set.
// NODE_ID is not used by PanelBridge (it is the CAN master, not a sub-node).

#include <DcsBios.h>
#include <PanelBridge.h>

void setup() {
    PanelBridge::setup(Serial2);  // Serial2 = UART2 PA2/PA3; arg removed in Phase 2
    DcsBios::setup();
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();
}
