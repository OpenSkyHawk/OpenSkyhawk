// E2E_DCS_Test — PanelGroup (STM32F103CBT6, NODE_ID=1)
//
// LED:    PC13 (built-in, active-LOW — dev-board shortcut; prefer external LED in production)
//         → A_4E_C_D_GLARE_WHEELS (Glareshield Wheels light — illuminates during Master Test)
// Button: PB0 (active-LOW; 10kΩ pull-up to 3.3V required; jumper wire to GND)
//         → DCSIN_MASTER_TEST

#include <OpenSkyhawk.h>

const PinRef PIN_LED(PC13);
const PinRef PIN_BTN(PB0);

OpenSkyhawk::LED        wheelsLight (A_4E_C_D_GLARE_WHEELS, A_4E_C_D_GLARE_WHEELS_AM, PIN_LED, /*reverse=*/true);
OpenSkyhawk::Switch2Pos masterTest  (DCSIN_MASTER_TEST, PIN_BTN);

void setup() {
    STM32Board::setDebug(true);
    PanelGroup::setup();
}

void loop() {
    PanelGroup::loop();
}
