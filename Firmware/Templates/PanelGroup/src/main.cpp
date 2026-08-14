// PanelGroup — STM32F103CBT6
//
// CAN sub-node. NODE_ID is set in platformio.ini build_flags, not here.
// A #define in main.cpp would be invisible to library translation units.
//
// Reads switches and analog inputs; sends CAN EVT {controlId, value} frames.
// Receives CAN CTRL_BCAST frames and drives outputs (LEDs, dimmers, steppers).
// controlId range determines routing at PanelBridge:
//   DCSIN_* (0x8001–0x86FF) → DCS-BIOS sendDcsBiosMessage()
//   CTRL_*  (0x0010–0x00FF) → HID frame → SimGateway → USB HID report

#include <OpenSkyhawk.h>

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
// #include <Outputs/LED/LED.h>
// OpenSkyhawk::LED <name>(A_4E_C_<ID>, A_4E_C_<ID>_AM, PIN_<NET>);
// OpenSkyhawk::LED <name>(A_4E_C_<ID>, A_4E_C_<ID>_AM, PIN_<NET>, /*reverse=*/true);

// ── Inputs → DCS-BIOS (DCSIN_* controlIds, 0x8001–0x86FF) ───────────────────
// OpenSkyhawk::Switch2Pos     <name>(DCSIN_<ID>, PIN_<NET>);
// OpenSkyhawk::Switch3Pos     <name>(DCSIN_<ID>, PIN_<NET_A>, PIN_<NET_B>);
// OpenSkyhawk::AnalogInput    <name>(DCSIN_<ID>, PIN_<NET>);
//
// NOT IMPLEMENTED YET — planned. Momentary press-only control: sends 1 on press, no release
// EVT, 20 ms debounce. Spec in FirmwarePlan/05-panelgroup-api.md, tracked as an open box in
// 10-implementation-plan.md. Switch2Pos is not a substitute — it emits on both edges.
// OpenSkyhawk::ActionButton   <name>(DCSIN_<ID>, PIN_<NET>);
//
// N-position selector. `pins` is a pointer to a caller-owned array that must outlive the
// object, so declare it alongside the wiring map. PIN_NC marks a mechanical-only detent.
// const PinRef PINS_<NAME>[] = { PIN_<NET_0>, PIN_<NET_1>, PIN_<NET_2> };
// OpenSkyhawk::SwitchMultiPos <name>(DCSIN_<ID>, PINS_<NAME>, 3);
//
// One pin, resistor ladder — for selectors with more positions than spare GPIO.
// OpenSkyhawk::AnalogMultiPos <name>(DCSIN_<ID>, PIN_<NET>, <numPos>);
//
// Rel = sends a signed delta per detent; Dir = sends INC/DEC. Pick per what the DCS
// control expects — a variable value wants Rel, a fixed-step frequency knob wants Dir.
// OpenSkyhawk::RotaryEncoder  <name>(DCSIN_<ID>, PIN_<NET_A>, PIN_<NET_B>,
//                                    OpenSkyhawk::EncoderStepsPerDetent::Four,
//                                    OpenSkyhawk::EncoderMode::Rel, /*step=*/1600);

// ── Inputs → HID (CTRL_* controlIds, 0x0010–0x00FF) ─────────────────────────
// OpenSkyhawk::Switch2Pos  <name>(CTRL_<ID>, PIN_<NET>);   // routes to HID button
// OpenSkyhawk::AnalogInput <name>(CTRL_<ID>, PIN_<NET>);   // routes to HID axis
//
// AnalogInput's defaults are ported from DCS-BIOS PotentiometerEWMA and are sized for a
// cockpit pot on a bandwidth-constrained serial link: 128 counts of hysteresis and an 8 ms
// read throttle (125 Hz/axis). The HID path has far more headroom, and a flight control
// wants finer steps than a volume knob:
//
//   OpenSkyhawk::AnalogInput <name>(CTRL_<ID>, PIN_<NET>, /*reverse=*/false, 0, 65535,
//                                   /*hysteresis=*/64);
//
// 64 halves the motion step size and still calibrates cleanly (measured on the bench rig;
// 8 does not — rest noise crosses the threshold constantly and the calibration hold never
// completes). Raise it instead — 1024 — for a control whose dispatch rate should stay calm.
//
// Hysteresis is a *change* threshold on the smoothed value, so a still axis emits nothing at
// all. Silence is the resting state, not a fault.

void setup() {
    // Bench only — REMOVE before flying. Every input's emit() then blocks on a ~20-char
    // USART1 write at 115200, which costs ~1.8 ms per emission and caps the whole node's
    // dispatch rate around 285 Hz regardless of anything else.
    STM32Board::setDebug(true);
    // Wire.begin();              // uncomment if using MCP23017 or ADS1115
    // PanelGroup::registerExpander(exp1, <INTA_pin>, <INTB_pin>);  // interrupt-driven
    // PanelGroup::registerExpander(exp1);                           // polling fallback
    // PanelGroup::registerADC(adc1, 0x48, Wire);
    PanelGroup::setup();
}

void loop() {
    PanelGroup::loop();
}
