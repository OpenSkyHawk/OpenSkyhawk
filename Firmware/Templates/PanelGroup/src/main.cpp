// PanelGroup — STM32F103CBT6
//
// CAN sub-node. NODE_ID is set in platformio.ini build_flags, not here.
// A #define in main.cpp would be invisible to library translation units.
//
// Reads switches and analog inputs; sends CAN EVT {controlId, value} frames.
// Receives CAN CTRL_BCAST frames and drives outputs (LEDs, dimmers, steppers).
// controlId range determines routing at PanelBridge:
//   DCSIN_* (0x8001–0x86FF) → DCS-BIOS sendDcsBiosMessage()
//   CTRL_*  (0x0010–0x009F) → HID frame → SimGateway → Joystick

#include <STM32Board.h>
#include <PanelGroup.h>
#include <A4EC_CmdIds.h>     // generated: #define DCSIN_* constants
#include <A4EC_OutputIds.h>  // generated: #define A_4E_C_*_A address + _AM mask constants

// ── Hardware ──────────────────────────────────────────────────────────────────
// Declare MCP23017 expanders and ADS1115 ADCs here.
// Address and I2C bus are passed to registerADC/registerExpander — not to the
// constructor. ADS1115 takes address via begin(addr, wire) (Adafruit v2 API).
// MCP23017 exp1(0x20, Wire);
// ADS1115   adc1;

// ── Wiring map ────────────────────────────────────────────────────────────────
// One PinRef per net label from the schematic. No magic numbers below this section.
// const PinRef PIN_<NET>(PB0);                       // direct STM32 GPIO
// const PinRef PIN_<NET>(exp1, PORT_A, 0);           // MCP23017 expander pin
// const PinRef PIN_<NET>(adc1, 0);                   // ADS1115 channel

// ── Outputs (DCS → hardware) ─────────────────────────────────────────────────
// #include <LED.h>
// OpenSkyhawk::LED <name>(A_4E_C_<ID>_A, A_4E_C_<ID>_AM, PIN_<NET>);
// OpenSkyhawk::LED <name>(A_4E_C_<ID>_A, A_4E_C_<ID>_AM, PIN_<NET>, /*reverse=*/true);

// ── Inputs → DCS-BIOS (DCSIN_* controlIds, 0x8001–0x86FF) ───────────────────
// OpenSkyhawk::Switch2Pos     <name>(DCSIN_<ID>, PIN_<NET>);
// OpenSkyhawk::Switch3Pos     <name>(DCSIN_<ID>, PIN_<NET_0>, PIN_<NET_1>);
// OpenSkyhawk::SwitchMultiPos <name>(DCSIN_<ID>, <n>, {PIN_<NET_0>, ...});
// OpenSkyhawk::ActionButton   <name>(DCSIN_<ID>, PIN_<NET>);
// OpenSkyhawk::AnalogInput    <name>(DCSIN_<ID>, PIN_<NET>);

// ── Inputs → HID (CTRL_* controlIds, 0x0010–0x009F) ─────────────────────────
// OpenSkyhawk::Switch2Pos  <name>(CTRL_<ID>, PIN_<NET>);   // routes to HID button
// OpenSkyhawk::AnalogInput <name>(CTRL_<ID>, PIN_<NET>);   // routes to HID axis

void setup() {
    STM32Board::setDebug(true);   // remove in production
    // Wire.begin();              // uncomment if using MCP23017 or ADS1115
    // PanelGroup::registerExpander(exp1, <INTA_pin>, <INTB_pin>);  // interrupt-driven
    // PanelGroup::registerExpander(exp1);                           // polling fallback
    // PanelGroup::registerADC(adc1, 0x48, Wire);
    PanelGroup::setup();
}

void loop() {
    PanelGroup::loop();
}
