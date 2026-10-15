// SoakRig — board B (STM32F103C8 on a PanelGroup base Rev 0.2.0, NODE_ID=2)
// PLACEHOLDER — pin and bit assignments, DCS bindings and all stepper/sensor calibration numbers
// below are provisional and expected to move once the panel is physically wired. What must NOT
// drift is the class-to-backend pairing: that is the coverage contract #291's gate checks.
//
// The second of the two permanent nodes behind the #291 soak test panel: 12 of the 24 stations.
// Board B REQUIRES a 0.2.0 base — J_SR does not exist on 0.1.0, and everything ShiftBus lives
// here. The rest of the rig's classes are on board A.
//
// Three X27.589 needles, one per PinRef backend, each answering a different question:
//   * MCP23017  — the only stepper configuration that SHIPS today (APN-153's DRIFT needle, chosen
//                 because that needle is slow: hardware-standards puts slow needles ≈163 °/s on
//                 I²C-class). This is the one to put under the soak.
//   * '595      — the FAST path: 1,137 steps/s measured in #197 gate 2, and what the standard
//                 names for gauges above ≈150 °/s. No board drives coils from a shift register
//                 yet, so this is where that gets proven before a board commits copper.
//   * direct GPIO — no shipping consumer at all (the only other DRV8833 board, Armament_MCU, is
//                 deprecated). Kept as a diagnostic baseline that isolates StepperMotor from an
//                 expander fault, and because this board's J11 is otherwise idle. First station
//                 to cut if those pins are ever needed.
//
// Wiring assumptions, from #291's construction notes:
//   * EXTERNAL 10 kΩ pull-ups on every input; bussed 10 k bias on ALL eight '165 inputs.
//   * 33 Ω series on SCK / LOAD / LATCH. '595 LEDs ≤ 4 mA direct with series R.
//   * DRV8833 VM = 5 V (absolute max 10.8 V — never the 12 V rail). Fit the module's EEP→VCC
//     jumper: the generic board's silk is backwards and nSLEEP floats otherwise.
//   * J11 position 11 is PC14 on 0.2.0 — an OSC32 pin with ~3 mA of drive. No outputs there; the
//     coils below use PA0–PA3 instead.
//
//   J11 (4 GPIO)            PA0–PA3 = X27 coils (direct-GPIO needle)
//   J_BL1                   PA6 = PWM_BL1 → Q2 → 12 V LED string  (sync pair with board A)
//   J_I2C1  (PB6/PB7)       MCP23017 @0x20 (X27 coils on port B) + TCA9548A @0x70
//                           + 2× SSD1306 @0x3C behind the mux
//   J_I2C2  (PB10/PB11)     1× SSD1306 @0x3C direct — bare 0x3C may not share the mux's trunk
//   J_SR    (PB3/4/5/8/9)   '165: encoder, ON-OFF-ON, stepper home switch
//                           '595: 4 caution LEDs + X27 coils
//
// ShiftBus needs no setup code at all: declaring an SR-backed PinRef is the whole opt-in, and
// PanelGroup::setup() begin()s the bus. '165 inputs and '595 outputs are indexed separately, so
// chip 0 bit 4 as an input (home switch) and chip 0 bit 4 as an output (a coil) are different pins.

#include <Wire.h>
#include <MCP23017.h>
#include <OpenSkyhawk.h>
#include <DrumDisplay.h>            // separate opt-in library (not in the OpenSkyhawk umbrella)
#include <Helpers/I2cMux/I2cMux.h>
#include <Helpers/ShiftBus/ShiftBus.h>

using namespace OpenSkyhawk;

// J_I2C2. The STM32 core does not predefine Wire1 for this variant, so the sketch declares it —
// which is also the symbol U8g2's _2ND_HW_I2C constructors bind to.
TwoWire Wire1(PB11, PB10);   // SDA=PB11, SCL=PB10

MCP23017 exp1(0x20, Wire);

// ── Needle 1 — direct GPIO (diagnostic baseline) ──────────────────────────────
// Canopy position is a full 16-bit value that sweeps while the canopy travels: easy to drive
// parked, and a genuine continuous-value check. STALL homing — no sensor on this one.
static const StepperConfig CANOPY_MOTOR =
    makeX27Config(/*home*/0, /*park*/0, /*minPos*/0, /*maxPos*/300);
StepperMotor canopyMotor(PinRef(PA0), PinRef(PA1), PinRef(PA2), PinRef(PA3), CANOPY_MOTOR);
static const GaugeCal CANOPY_CAL = { 0, 300, /*reverse=*/false, nullptr, nullptr, 0 };
NeedleGauge  canopyNeedle(A_4E_C_CANOPY_POS, A_4E_C_CANOPY_POS_AM, canopyMotor, CANOPY_CAL);

#if 0
// StepPattern::FULL_4STATE — a REFLASH of the needle above, not extra hardware.
// #295 freezes the API at 1.0.0, and this enum member would otherwise ship unexercised. Driving
// an X27 with the 4-state table proves the TABLE and the SEQUENCER, NOT the geared-motor profile
// it is meant for — say so in the result. Swap CANOPY_MOTOR for this config to run it.
//
// makeX27Config() cannot express this: it hardcodes StepPattern::SWITEC_6STATE
// (StepperMotor.cpp:263) and exposes no pattern parameter, so the field must be overwritten after
// the fact. Passing the remaining arguments explicitly would only restate their defaults and
// would still leave the pattern at 6-state — i.e. an alternate build that silently ran the main
// build's drive sequence and reported FULL_4STATE as proven.
//
// stepsPerRev stays at the X27 default, which under a 4-state table no longer describes this
// motor, so the needle's travel will be wrong. That is expected and is not what this build
// measures.
static StepperConfig makeCanopy4StateConfig() {
    StepperConfig cfg = makeX27Config(/*home*/0, /*park*/0, /*minPos*/0, /*maxPos*/300);
    cfg.pattern = StepPattern::FULL_4STATE;
    return cfg;
}
static const StepperConfig CANOPY_MOTOR_4STATE = makeCanopy4StateConfig();
#endif

// ── Needle 2 — MCP23017 port B (the configuration that ships) ─────────────────
// Four coil pins on ONE port so each step is a single batched writePort() with an atomic coil
// flip — ~490 steps/s at 400 kHz, bus-limited. Moved here from #137.
static const StepperConfig HOOK_MOTOR =
    makeX27Config(/*home*/0, /*park*/0, /*minPos*/0, /*maxPos*/300);
StepperMotor hookMotor(PinRef(exp1, 1, 0), PinRef(exp1, 1, 1),
                       PinRef(exp1, 1, 2), PinRef(exp1, 1, 3), HOOK_MOTOR);
static const GaugeCal HOOK_CAL = { 0, 300, /*reverse=*/false, nullptr, nullptr, 0 };
NeedleGauge  hookNeedle(A_4E_C_EXT_HOOK, A_4E_C_EXT_HOOK_AM, hookMotor, HOOK_CAL);

// ── Needle 3 — '595 ShiftBus, homed on a '165 microswitch ─────────────────────
// The fast path, and the only station that exercises HomeMode::SENSOR — the other enum member
// #295 would otherwise freeze unproven. The home switch is NOT a faceplate feature: it mounts
// behind this needle and is wired back to the '165.
static const StepperConfig FLAPS_MOTOR =
    makeX27Config(/*home*/0, /*park*/0, /*minPos*/0, /*maxPos*/300, HomeMode::SENSOR);
StepperMotor flapsMotor(PinRef(ShiftBus1, 0, 4), PinRef(ShiftBus1, 0, 5),
                        PinRef(ShiftBus1, 0, 6), PinRef(ShiftBus1, 0, 7), FLAPS_MOTOR,
                        /*homeSense=*/PinRef(ShiftBus1, 0, 4));   // '165 bit 4 — separate chain
static const GaugeCal FLAPS_CAL = { 0, 300, /*reverse=*/false, nullptr, nullptr, 0 };
NeedleGauge  flapsNeedle(A_4E_C_D_FLAPS_IND, A_4E_C_D_FLAPS_IND_AM, flapsMotor, FLAPS_CAL);

// ── J_SR '165 — inputs ────────────────────────────────────────────────────────
// Encoder on the ISR-sampled path (SHIFTBUS_ISR_HZ): loop-polling captured only ~34 % of
// fast-spin detents under a 25 ms stall, the 1 kHz ISR captured 100 %.
// RotaryAcceleratedEncoder in Rel mode is #287's acceptance station: a fast spin must send
// fastStep, and a reverse blip mid-spin must be swallowed by the momentum filter.
// step 1600 (½ the JSON suggested_step) = 1 digit per detent on the DEST_LAT readout.
RotaryAcceleratedEncoder destLat(DCSIN_DEST_LAT_KNB, PinRef(ShiftBus1, 0, 0),
                                 PinRef(ShiftBus1, 0, 1), EncoderStepsPerDetent::Four,
                                 /*step=*/1600, /*fastStep=*/6400);

// ON-OFF-ON → navigation lights. LIGHT_EXT_NAV's 2-bit mask (0x00c0) is a genuine 3-state
// switch, so this is a real class fit rather than a position left unused. Confirm in F2.
Switch3Pos navLights(DCSIN_LIGHT_EXT_NAV, PinRef(ShiftBus1, 0, 2), PinRef(ShiftBus1, 0, 3));

// ── J_SR '595 — outputs ───────────────────────────────────────────────────────
// The caution ladder is ONE word at 0x845c with a mask per lamp, so board A's master-test press
// lights all four of these together with its own GPIO and MCP LEDs — multi-mask fan-out from a
// single broadcast, which nothing else on the rig does.
LED srUtilHyd (A_4E_C_D_UTILHYD_CAUTION,  A_4E_C_D_UTILHYD_CAUTION_AM,  PinRef(ShiftBus1, 0, 0));
LED srContHyd (A_4E_C_D_CONTHYD_CAUTION,  A_4E_C_D_CONTHYD_CAUTION_AM,  PinRef(ShiftBus1, 0, 1));
LED srFuelBst (A_4E_C_D_FUELBOOST_CAUTION,A_4E_C_D_FUELBOOST_CAUTION_AM,PinRef(ShiftBus1, 0, 2));
LED srRadarWarn(A_4E_C_D_RADAR_WARN,      A_4E_C_D_RADAR_WARN_AM,       PinRef(ShiftBus1, 0, 3));

// ── J_BL1 — PWM backlight zone (sync pair with board A) ───────────────────────
// Bound to the SAME value board A's pot drives. Both nodes receive that broadcast independently
// and must light to the same brightness; divergence between them is a decode or timing fault that
// no single-node station can show. The direct OLED below displays the same value as a number, so
// the expected duty is readable next to the two actual brightnesses.
Dimmer instrZone(A_4E_C_LIGHT_INT_INSTR, PinRef(PA6));

// ── Drum readouts ─────────────────────────────────────────────────────────────
// Whole-word 0xFFFF sources: one source feeding all five digit columns from place 0.
static const DrumSource CANOPY_SRC[] = {
    { A_4E_C_CANOPY_POS, A_4E_C_CANOPY_POS_AM, 5, 0 },
};
static const DrumReadout CANOPY_READOUT = {
    .sources = CANOPY_SRC, .nSources = 1, .nDigits = 5,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
};

static const DrumSource HOOK_SRC[] = {
    { A_4E_C_EXT_HOOK, A_4E_C_EXT_HOOK_AM, 5, 0 },
};
static const DrumReadout HOOK_READOUT = {
    .sources = HOOK_SRC, .nSources = 1, .nDigits = 5,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
};

static const DrumSource INSTR_SRC[] = {
    { A_4E_C_LIGHT_INT_INSTR, A_4E_C_LIGHT_INT_INSTR_AM, 5, 0 },
};
static const DrumReadout INSTR_READOUT = {
    .sources = INSTR_SRC, .nSources = 1, .nDigits = 5,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
};

// Two panels behind the TCA9548A on I2C1 (Wire) …
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C oledCanopy(U8G2_R0, U8X8_PIN_NONE);
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C oledHook  (U8G2_R0, U8X8_PIN_NONE);
I2cMux drumMux(0x70, Wire);

DrumDisplay canopyDrum(oledCanopy, CANOPY_READOUT, drumMux, /*channel*/ 0, DrumFont::LARGE);
DrumDisplay hookDrum  (oledHook,   HOOK_READOUT,   drumMux, /*channel*/ 1, DrumFont::LARGE);

// … and one on a BARE bus — the DrumDisplay path with no I2cMux in front of it.
//
// It sits on I2C2, the trunk WITHOUT the mux: hardware-standards forbids a bare 0x3C device on a
// trunk that also carries 0x3C behind a mux, because it is shadowed whenever a channel is open.
// The bus is passed to the ctor so the reachability probe talks to I2C2 too — the U8G2 object
// carries its own bus, and without a matching handle the probe would test I2C1 and the health
// breaker would fault a working panel.
U8G2_SSD1306_128X32_UNIVISION_F_2ND_HW_I2C oledInstr(U8G2_R0, U8X8_PIN_NONE);
DrumDisplay instrDrum(oledInstr, INSTR_READOUT, Wire1, DrumFont::LARGE);

void setup() {
    STM32Board::setDebug(true);

    // I2C1 = PB6/PB7 on the PanelGroup base — the STM32 default, no remap. (E2E_DCS_Test remaps
    // I2C1 to PB8/PB9 for a bare Blue Pill; on a base board those two pins are the ShiftBus
    // SR_LOAD / SR_LATCH, which this node uses.)
    Wire.begin();
    Wire1.begin();

    // Each OLED must begin() on its own mux channel BEFORE PanelGroup::setup() calls configure()
    // on every output.
    drumMux.select(0);
    oledCanopy.setI2CAddress(0x3C << 1);
    oledCanopy.begin();
    drumMux.select(1);
    oledHook.setI2CAddress(0x3C << 1);
    oledHook.begin();

    // The bare-bus panel on I2C2 has no channel to select.
    oledInstr.setI2CAddress(0x3C << 1);
    oledInstr.begin();

    PanelGroup::registerExpander(exp1, PB12, PB13);  // INT_A → PB12, INT_B → PB13 (J_I2C1)

    PanelGroup::setup();   // boots the board + CAN, begin()s ShiftBus1, configure()s every control
}

void loop() {
    PanelGroup::loop();    // polls inputs, dispatches CTRL_BCAST, drives update() on the drums
}
