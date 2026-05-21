// CAN_Test_SubNode — STM32F103CBT6
//
// Node ID from PA0 strap: HIGH (tied to 3.3V) → node_id=1,
//                         LOW  (internal pull-down) → node_id=2.
//
// CAN1 (PA11 RX / PA12 TX) @ 500 kbps.
// DiagSerial: USART1 PA9 TX / PA10 RX @ 115200.
//
// PB1 button: sends INPUT_EVENT (controlId 0x0001) to CAN bus on state change.
// Heartbeat and CAN health managed by PanelGroup::loop().

#include <PanelGroup.h>

static constexpr uint8_t BTN_PIN = PB1;
static bool lastBtnState = HIGH;

static void checkButton() {
    bool state = digitalRead(BTN_PIN);
    if (state == lastBtnState) return;
    lastBtnState = state;
    PanelGroup::sendEvent(0x0001, (state == LOW) ? 1 : 0);
}

void setup() {
    pinMode(BTN_PIN, INPUT_PULLUP);
    PanelGroup::setup();
}

void loop() {
    PanelGroup::loop();
    checkButton();
}
