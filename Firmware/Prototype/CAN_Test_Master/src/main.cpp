// CAN_Test_Master — STM32F103CBT6
//
// Serial = UART2 (PA2/PA3) @ 250000 — RP2040 SimGateway link
// CAN1   PA11/PA12 @ 500 kbps — SN65HVD230 transceiver

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>

void setup() {
    PanelBridge::setup();
    DcsBios::setup();
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();
}
