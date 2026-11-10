// PanelGroupSample — STM32F103C8
//
// A small, working PanelGroup node: one of each common control class, wired straight to the
// STM32's own pins, so it runs on a bare PanelGroup base board with nothing else fitted. The controls are the ones on the Getting Started → Controls pages.
//
// NODE_ID is set in platformio.ini build_flags, not here.
// A #define in main.cpp would be invisible to library translation units.
//
// For the full menu of classes (expanders, ADCs, HID routing, AnalogInput tuning), start from
// Firmware/Templates/PanelGroup instead.

#include <OpenSkyhawk.h>

// ── Wiring map ────────────────────────────────────────────────────────────────
// Every pin is on the base board's J_BREAKOUT header (pin number in brackets). Switches, the
// button and the encoder pull their pin to GND and need a 10 kΩ pull-up to 3.3V on each pin.
// Left free: PB2 [7] (BOOT1) and PC14 [11] (RTC domain — not for general I/O).
const PinRef MASTER_ARM_PIN  = PinRef(PB0);   // [3]  2-position toggle
const PinRef VOLUME_KNOB_PIN = PinRef(PA0);   // [4]  potentiometer wiper (ends on 3.3V / GND)
const PinRef TAXI_LIGHT_PIN  = PinRef(PB1);   // [5]  momentary push button
const PinRef DEST_LAT_A_PIN  = PinRef(PA1);   // [6]  rotary encoder A (C to GND)
const PinRef DEST_LAT_B_PIN  = PinRef(PA2);   // [8]  rotary encoder B
const PinRef FIRE_LIGHT_PIN  = PinRef(PA5);   // [9]  LED via 330 Ω to GND, lit when HIGH
const PinRef BOMB_ARM_PIN_A  = PinRef(PA3);   // [10] 3-position toggle, one end
const PinRef BOMB_ARM_PIN_B  = PinRef(PA4);   // [12] 3-position toggle, other end

// ── Outputs (DCS → hardware) ─────────────────────────────────────────────────
OpenSkyhawk::LED fireLight(A_4E_C_D_GLARE_FIRE, A_4E_C_D_GLARE_FIRE_AM, FIRE_LIGHT_PIN);

// ── Inputs → DCS-BIOS ────────────────────────────────────────────────────────
OpenSkyhawk::Switch2Pos    masterArm(DCSIN_ARM_MASTER, MASTER_ARM_PIN);
OpenSkyhawk::Switch3Pos    bombArm(DCSIN_ARM_BOMB, BOMB_ARM_PIN_A, BOMB_ARM_PIN_B);
OpenSkyhawk::ActionButton  taxiLight(DCSIN_LIGHT_EXT_TAXI, TAXI_LIGHT_PIN);
OpenSkyhawk::AnalogInput   volume(DCSIN_ARC51_VOL, VOLUME_KNOB_PIN);
OpenSkyhawk::RotaryEncoder destLat(DCSIN_DEST_LAT_KNB, DEST_LAT_A_PIN, DEST_LAT_B_PIN,
                                   OpenSkyhawk::EncoderStepsPerDetent::Four);

// ── Adding an MCP23017 expander ──────────────────────────────────────────────
// Same classes, the pin just lives on the expander. Wire it to I2C1 (PB6/PB7) and:
// MCP23017 exp1(0x20, Wire);
// const PinRef GEAR_PIN = PinRef(exp1, PORT_A, 0);
// OpenSkyhawk::Switch2Pos gear(DCSIN_<ID>, GEAR_PIN);
// …then Wire.begin() and PanelGroup::registerExpander(exp1) in setup(), before PanelGroup::setup().

void setup() {
    // DiagSerial log off by default (#389). Turn it on for the bench only: every input's emit()
    // then blocks on a ~20-char USART1 write at 115200, which costs ~1.8 ms per emission and caps
    // the whole node's dispatch rate around 285 Hz regardless of anything else.
    // STM32Board::setDebug(true);
    PanelGroup::setup();
}

void loop() {
    PanelGroup::loop();
}
