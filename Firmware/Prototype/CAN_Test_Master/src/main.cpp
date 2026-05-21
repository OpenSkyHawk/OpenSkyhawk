// CAN_Test_Master — STM32F103CBT6
//
// USART2 PA2/PA3 @ 250000 — RP2040 SimGateway link (Serial2, built-in)
// CAN1   PA11/PA12 @ 500 kbps — SN65HVD230 transceiver
//
// Transparent bridge: ControlPacket UART ↔ CAN broadcast.
// Heartbeat monitoring and DIAG frame forwarding handled by PanelBridge.

#include <PanelBridge.h>

void setup() {
    PanelBridge::setup(Serial2);
}

void loop() {
    PanelBridge::loop();
}
