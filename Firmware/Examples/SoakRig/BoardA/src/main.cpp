// SoakRig — board A (STM32F103C8 on a PanelGroup base, NODE_ID=1)
// PLACEHOLDER — pin and bit assignments, DCS bindings and all stepper/sensor calibration numbers
// below are provisional and expected to move once the panel is physically wired. What must NOT
// drift is the class-to-backend pairing: that is the coverage contract #291's gate checks.
//
// One of the two permanent nodes behind the #291 soak test panel. Between them the two boards
// carry one instance of every v1 control class on every PinRef backend — 24 stations, 12 here.
// This is a TEST INSTRUMENT, not a replica panel: each station is bound to whichever DCS control
// is easiest to drive and confirm, which is mostly lighting, because those switches work cold and
// parked and read back from the cockpit or an F2 external view. Nothing here needs to be airborne.
//
// Board A runs on a 0.1.0 OR a 0.2.0 base: every J11 pin used below exists on both revisions
// (positions 7 and 11 — PB5/PA8 on 0.1.0, PB2/PC14 on 0.2.0 — are deliberately left alone).
// Board B is the one that needs 0.2.0, for J_SR.
//
// Wiring assumptions, from #291's construction notes:
//   * EXTERNAL 10 kΩ pull-ups on every input. The firmware sets plain INPUT on GPIO and GPPU=0
//     on the MCP23017 — no internal bias anywhere.
//   * Analog RC (1 kΩ + 100 nF) at the connector end of each pot/ladder leg.
//   * MCP23017 GPA7/GPB7 are output-only (silicon erratum) → the LED lives on GPB7.
//
//   J11 (6 of 10 GPIO)      PA0 pot · PA1 resistor ladder · PA2/PA3 encoder · PA4 momentary
//                           PB0 LED  (timer-capable on purpose: this station can be reflashed
//                                     as a Dimmer without rewiring)
//   J_BL1                   PA6 = PWM_BL1 → Q2 → 12 V LED string
//   J_I2C1  (PB6/PB7)       MCP23017 @0x20, INT_A→PB12  INT_B→PB13
//   J_I2C2  (PB11/PB10)     ADS1115 @0x48 — the second trunk; see setup()
//
// Reflash variants are kept as `#if 0` blocks rather than extra hardware — see the ticket.

#include <Wire.h>
#include <MCP23017.h>
#include <OpenSkyhawk.h>

using namespace OpenSkyhawk;

// ── Expanders ─────────────────────────────────────────────────────────────────
// Registered in setup() before PanelGroup::setup(), which then configure()s every control.
TwoWire WireI2C2(PB11, PB10);   // J_I2C2 — SDA=PB11, SCL=PB10

MCP23017 exp1(0x20, Wire);
ADS1115  adc1;

// ── J11 — direct GPIO ─────────────────────────────────────────────────────────
// Pot → instrument lighting. Both Dimmer stations (this board's J_BL1 and board B's) read the
// value this pot writes, so turning it is the sync check: the two strings must track together.
// hysteresis 1024 (≈64 steps over the throw) — the 128 default floods DCS-BIOS import on a sweep.
AnalogInput    instrPot (DCSIN_LIGHT_INT_INSTR, PinRef(PA0), /*reverse=*/false, 0, 65535,
                         /*hysteresis=*/1024);

// Resistor-ladder rotary → the >12-position case for AnalogMultiPos. TACAN's major channel
// selector has a 4-bit mask (0x0780), so 16 positions; use as many as the physical switch has.
AnalogMultiPos tacanMaj (DCSIN_TACAN_CHAN_MAJ, PinRef(PA1), 16);

// Encoder → console-light intensity. LIGHT_INT_BRIGHT is a stepped 4-state control (mask 0x0c00),
// which is what DIR mode is for: each detent emits one INC/DEC rather than a variable delta.
RotaryEncoder  brightKnob(DCSIN_LIGHT_INT_BRIGHT, PinRef(PA2), PinRef(PA3),
                          EncoderStepsPerDetent::Four, EncoderMode::Dir);

// Momentary → master test. One sim toggle per press, nothing on release (#116).
ActionButton   masterTest(DCSIN_MASTER_TEST, PinRef(PA4));

#if 0
// ── Switch2Pos on the momentary — reflash of the station above, NOT extra hardware ──
// ActionButton and Switch2Pos are SIBLINGS off InputBase, not a hierarchy: ActionButton emits one
// EVT_ACTION per debounced press EDGE (payload 0 = TOGGLE, and the sim supplies the current
// value), while Switch2Pos sends absolute 0/1 from the LEVEL. (SwitchWithCover2Pos is the one that
// derives from Switch2Pos.) So the two cannot coexist on one pin, and this is the GPIO-backend
// instance of the Switch2Pos path — the MCP toggle above covers the expander backend.
//
// Binding Switch2Pos to a spring-return part is a deliberate rig compromise: the class header says
// it wants a part that LATCHES, because a momentary returns the control to 0 the moment it is
// released. Record the outcome as "class path proven, part not representative" — Switch2Pos itself
// was hardware-verified in Phase 4, so nothing unproven rides on it.
//
// This is also the fallback if the MASTER_TEST class check fails. #291 asks to confirm the mod
// models MASTER_TEST as a toggle (DCS-BIOS defineToggleSwitch) before leaving ActionButton on it;
// a held push-to-test is momentary_last_position, which is a plain Switch2Pos and nothing else.
Switch2Pos     masterTestHold(DCSIN_MASTER_TEST, PinRef(PA4));
#endif

// LED → one lamp of the caution ladder. The whole ladder is a single word at 0x845c with a mask
// per lamp, so pressing the master-test button lights this LED, the MCP LED below, and board B's
// '595 LEDs together — and exercises multi-mask fan-out from one broadcast.
LED            spoilerCaution(A_4E_C_D_SPOILER_CAUTION, A_4E_C_D_SPOILER_CAUTION_AM, PinRef(PB0));

// ── J_BL1 — PWM backlight zone ────────────────────────────────────────────────
// PA6 is PWM_BL1 → Q2 (TIM3_CH1) → the 12 V string. Dimmer requires a direct GPIO on a timer
// channel; an MCP or ShiftBus PinRef is refused and left disabled rather than degrading.
Dimmer         consoleZone(A_4E_C_LIGHT_INT_INSTR, PinRef(PA6));

// ── IntegerOutput — no hardware ───────────────────────────────────────────────
// Canopy position is a full 16-bit value that sweeps while the canopy travels, so it is both the
// easiest output to drive parked and a good continuous-value check. Board B also shows it on an
// OLED and drives a needle from it.
static void logCanopy(uint16_t value) {
    if (!STM32Board::isDebug()) return;
    char line[32];
    snprintf(line, sizeof(line), "CANOPY_POS %u", value);
    STM32Board::log(line);
}
IntegerOutput  canopyPos(A_4E_C_CANOPY_POS, logCanopy);

// ── J_I2C1 — MCP23017 @0x20 ───────────────────────────────────────────────────
// Encoder on the expander's interrupt path. ARC51_FREQ_1MHZ is a 4-bit selector (0x3c00), so DIR.
// Needs the radio powered to see the readout — the only station here that is not ramp-confirmable.
RotaryEncoder  freq1Mhz (DCSIN_ARC51_FREQ_1MHZ, PinRef(exp1, 0, 0), PinRef(exp1, 0, 1),
                         EncoderStepsPerDetent::Four, EncoderMode::Dir);

// 2-pos toggle → anti-collision lights. Confirm in F2.
// This station doubles as the SwitchWithCover2Pos check (#293) — see the note below.
Switch2Pos     antiColl (DCSIN_LIGHT_EXT_ANTICOLL, PinRef(exp1, 0, 2));

#if 0
// ── #293 SwitchWithCover2Pos — reflash of the toggle above, NOT extra hardware ──
// The class takes a SINGLE pin and fakes the cover in firmware: cover, 200 ms, then the switch;
// reversed for off. So it is electrically the same one input as Switch2Pos, and fitting a real
// guarded switch would add a part without adding anything testable — the class never reads a
// cover contact.
//
// The A-4E-C uses this class 0×. Its only cover, AFCS_1N2_COVER, guards a 3-POSITION switch that
// the mod does not gate on the cover, and that cover has its own DCSIN — so a guarded 3-pos needs
// no special class at all (Switch3Pos plus an optional Switch2Pos on the cover, both already on
// this rig). The class is kept purely for DCS-BIOS parity.
//
// Both constructor arguments are ordinary control ids; nothing requires the second to be a real
// cover. Since no 2-position A-4E control has a paired cover, the check pairs the two armament
// station selects: both single-bit, adjacent on the panel, and sharing broadcast word 0x8500
// (masks 0x0020 / 0x0040), so the 200 ms ordering is visible on screen AND as two bit changes in
// one word. Throw the toggle: station 1 flips, 200 ms later station 2 follows; off reverses.
SwitchWithCover2Pos coverPair(DCSIN_ARM_STATION2, DCSIN_ARM_STATION1, PinRef(exp1, 0, 2));
#endif

// 5-position rotary → nav mode select (3-bit mask 0x0038 = 8 states; 5 used). The switch animates
// in the cockpit when thrown, so the input confirms even before the set is powered.
// GPA7 is output-only, so these sit on port B.
static const PinRef NAV_SEL_PINS[] = {
    PinRef(exp1, 1, 0), PinRef(exp1, 1, 1), PinRef(exp1, 1, 2),
    PinRef(exp1, 1, 3), PinRef(exp1, 1, 4),
};
SwitchMultiPos navSel  (DCSIN_NAV_SEL, NAV_SEL_PINS, 5);

// LED on the expander → a second caution lamp, lit by the same master-test press.
LED            spdbrkCaution(A_4E_C_D_SPDBRK_CAUTION, A_4E_C_D_SPDBRK_CAUTION_AM,
                             PinRef(exp1, 1, 7));   // GPB7 — output-only by erratum

// ── ADS1115 @0x48 ─────────────────────────────────────────────────────────────
// Pot on the ADS path → white floodlight. Same class as the J11 pot, different backend.
AnalogInput    floodPot(DCSIN_LIGHT_INT_FLOOD_WHT, PinRef(adc1, 0), /*reverse=*/false, 0, 65535,
                        /*hysteresis=*/1024);

#if 0
// ── AngleSensorInput (#294) — NOT a station on this rig ───────────────────────
// AS5600 / MT6701 analog output → gunsight depression. Kept here because board A's ADS is where
// it would land, but #291 moved it OFF the faceplate: the magnet has to sit in a knob concentric
// with the axis of rotation, with the sensor behind it at a controlled airgap, which is a bespoke
// knob + bushing + mount rather than a COTS part in a hole. It gets its own small fixture, and
// #290's "live check of every in-scope class" is still met because the check happens there.
//
// Leave this disabled while the rig is soaking: nothing is wired to ADS channel 1, so a live
// instance reads a floating input and streams DCSIN_GUNSIGHT_KNB noise onto the bus for hours.
//
// centerDeg/travelDeg describe where the magnet sits and how far the knob turns; both are
// bench-tuned per build, and the values below are invented starting points. The acceptance check
// is a sweep across the full travel INCLUDING a pass through the sensor's 0°/360° wrap.
AngleSensorInput gunsightKnob(DCSIN_GUNSIGHT_KNB, PinRef(adc1, 1),
                              /*centerDeg=*/180.0f, /*travelDeg=*/90.0f);
#endif

void setup() {
    STM32Board::setDebug(true);

    // I2C1 = PB6/PB7 and I2C2 = PB10/PB11 on the PanelGroup base — these are the STM32 defaults,
    // so no remap. (The E2E_DCS_Test sketch remaps I2C1 to PB8/PB9 for a bare Blue Pill; on a base
    // board those two pins belong to the ShiftBus as SR_LOAD / SR_LATCH.)
    // This node uses BOTH trunks, per the rig's harness legend (work/291/rig-20N-layoutB.svg):
    // J_I2C1 carries the MCP23017 at 0x20, J_I2C2 carries the ADS1115 at 0x48. The split is the
    // harness's, not an electrical requirement — 0x20 and 0x48 would not collide on one trunk.
    //
    // The STM32 core does not predefine a second TwoWire for this variant, so WireI2C2 is declared
    // above and handed to registerADC(). PanelGroup::registerADC(adc, addr, bus) takes the bus
    // explicitly and PinRef abstracts the backend, so no control class knows which trunk it is on.
    Wire.begin();
    WireI2C2.begin();

    PanelGroup::registerExpander(exp1, PB12, PB13);  // INT_A → PB12, INT_B → PB13 (J_I2C1)
    PanelGroup::registerADC(adc1, 0x48, WireI2C2);

    PanelGroup::setup();   // boots the board + CAN, configure()s every registered control
}

void loop() {
    PanelGroup::loop();    // polls inputs, dispatches CTRL_BCAST to the outputs
}
