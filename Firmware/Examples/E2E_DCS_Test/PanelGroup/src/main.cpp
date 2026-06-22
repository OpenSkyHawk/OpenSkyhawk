// E2E_DCS_Test — PanelGroup node (STM32F103C8, NODE_ID=1)
//
// ONGOING integration node: one live instance of every control class we build, driven by real
// DCS-BIOS over CAN (SimGateway → PanelBridge → this node via CTRL_BCAST). Add new controls to
// the wiring-map sections below as they land; PanelGroup::setup()/loop() drive them all.
//
// Digital I/O:
//   LED    PC13 (built-in, active-LOW)            → A_4E_C_D_GLARE_WHEELS (Master Test lamp)
//   Button PB0  (active-LOW, 10kΩ pull-up to 3V3) → DCSIN_MASTER_TEST
// Needle gauge — X27 air-core stepper via DRV8833 (coils PA0/PA1/PA4/PA5):
//   APN-153 DRIFT needle ← A_4E_C_APN153_DRIFT_GAUGE (centre-zero)
// OLED drum readouts — TCA9548A @ 0x70 on I2C1 (SCL=PB8, SDA=PB9), each panel @ 0x3C:
//   ch0 → current longitude  (NAV_CURPOS_LON, 6 digits + E/W flag)
//   ch1 → ARC-51 UHF frequency (ARC51_FREQ, 5 digits + '.', e.g. 225.50)

#include <Wire.h>
#include <OpenSkyhawk.h>
#include <DrumDisplay.h>            // separate opt-in library (not in the OpenSkyhawk umbrella)
#include <Helpers/I2cMux/I2cMux.h>

using namespace OpenSkyhawk;

// ── Digital I/O ───────────────────────────────────────────────────────────────
const PinRef PIN_LED(PC13);
const PinRef PIN_BTN(PB0);

LED        wheelsLight(A_4E_C_D_GLARE_WHEELS, A_4E_C_D_GLARE_WHEELS_AM, PIN_LED, /*reverse=*/true);
Switch2Pos masterTest (DCSIN_MASTER_TEST, PIN_BTN);

// ── APN-153 DRIFT needle — X27 air-core stepper via DRV8833 ─────────────────────
// Centre-zero gauge: DCS 0 → full-left, 32768 → centre, 65535 → full-right. NeedleGauge
// auto-registers like the LED/drums, so PanelGroup::setup() homes it and loop() drives it
// from CTRL_BCAST — the needle tracks live DRIFT data over CAN with no extra code here.
// Homing: into the full-left stop (assigned -150), then park at centre (0); home direction /
// offset are per-gauge bench-tuned. DRV8833 ~SLEEP tied HIGH in hardware (or wire to a spare
// GPIO + pass as the StepperMotor sleepEn arg). makeX27Config bakes the air-core defaults
// (1080 steps/rev, rangeSteps 945, gentle homing); set rangeSteps 960 for a 320° BKA-30.
static const StepperConfig DRIFT_MOTOR =
    makeX27Config(/*home*/-150, /*park*/0, /*minPos*/-150, /*maxPos*/150);
StepperMotor driftMotor(PinRef(PA0), PinRef(PA1), PinRef(PA4), PinRef(PA5), DRIFT_MOTOR);
static const GaugeCal DRIFT_CAL = { -150, 150, /*reverse=*/false, nullptr, nullptr, 0 };
NeedleGauge driftGauge(A_4E_C_APN153_DRIFT_GAUGE, A_4E_C_APN153_DRIFT_GAUGE_AM,
                       driftMotor, DRIFT_CAL);

// ── OLED drum readouts (two panels behind a TCA9548A @ 0x70 on I2C1) ────────────
U8G2_SH1106_128X64_NONAME_F_HW_I2C oledLon  (U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C oledRadio(U8G2_R0, U8X8_PIN_NONE);
I2cMux drumMux(0x70, Wire);

// Current longitude — 6 digits + E/W hemisphere flag.
// TODO(bench): the rightmost source carries the ones digit and/or the E/W hemisphere — confirm
// against live DCS-BIOS; this node is the place to settle it.
static const DrumSource LON_SRC[] = {
    { A_4E_C_NAV_CURPOS_LON_X00000, A_4E_C_NAV_CURPOS_LON_X00000_AM, 1, 5 },
    { A_4E_C_NAV_CURPOS_LON_0X0000, A_4E_C_NAV_CURPOS_LON_0X0000_AM, 1, 4 },
    { A_4E_C_NAV_CURPOS_LON_00X000, A_4E_C_NAV_CURPOS_LON_00X000_AM, 1, 3 },
    { A_4E_C_NAV_CURPOS_LON_000X00, A_4E_C_NAV_CURPOS_LON_000X00_AM, 1, 2 },
    { A_4E_C_NAV_CURPOS_LON_0000X0, A_4E_C_NAV_CURPOS_LON_0000X0_AM, 1, 1 },
    { A_4E_C_NAV_CURPOS_LON_00000X, A_4E_C_NAV_CURPOS_LON_00000X_AM, 1, 0 },
};
static const DrumReadout LON_READOUT = {
    LON_SRC, 6, 6, 4.5f, 8.0f, 1.0f, 0.0f, 0, nullptr, 0,
    { true, A_4E_C_NAV_CURPOS_LON_00000X, A_4E_C_NAV_CURPOS_LON_00000X_AM, "EW", 6, 5.5f },
    DrumScroll::SNAP_SETTLE, 3.0f,
};

// ARC-51 UHF displayed frequency — 5 digits (2+1+2) + '.' (the value the MHz selector sets).
// TODO(bench): confirm the grouped 00–99 encoding and the dot position against the real panel.
// (Raw manual-selector knobs live at A_4E_C_ARC51_FREQ_10MHZ/_1MHZ/_50KHZ, bit-packed — swap to
//  those if you want the knob positions instead of the displayed frequency.)
static const DrumSource ARC51_SRC[] = {
    { A_4E_C_ARC51_FREQ_XX000, A_4E_C_ARC51_FREQ_XX000_AM, 2, 3 },
    { A_4E_C_ARC51_FREQ_00X00, A_4E_C_ARC51_FREQ_00X00_AM, 1, 2 },
    { A_4E_C_ARC51_FREQ_000XX, A_4E_C_ARC51_FREQ_000XX_AM, 2, 0 },
};
static const DrumGlyph ARC51_DOT[] = { { '.', 3, 1.8f } };
static const DrumReadout ARC51_READOUT = {
    ARC51_SRC, 3, 5, 4.5f, 8.0f, 1.0f, 0.0f, 0, ARC51_DOT, 1,
    { false, 0, 0, nullptr, 0, 0.0f },
    DrumScroll::SNAP_SETTLE, 3.0f,
};

DrumDisplay lonDrum  (oledLon,   LON_READOUT,   drumMux, /*channel*/ 0, DrumFont::LARGE);
DrumDisplay radioDrum(oledRadio, ARC51_READOUT, drumMux, /*channel*/ 1, DrumFont::LARGE);

void setup() {
    STM32Board::setDebug(true);

    // Bring each OLED up on its own mux channel BEFORE PanelGroup::setup() (which calls
    // configure() on every output): begin() must precede configure(), each on its channel.
    Wire.setSCL(PB8);
    Wire.setSDA(PB9);
    Wire.begin();
    drumMux.select(0);
    oledLon.setI2CAddress(0x3C << 1);
    oledLon.begin();
    drumMux.select(1);
    oledRadio.setI2CAddress(0x3C << 1);
    oledRadio.begin();

    PanelGroup::setup();   // boots the board + CAN, calls configure() on all registered controls
}

void loop() {
    PanelGroup::loop();    // dispatches CTRL_BCAST → onControlPacket; drives update() on the drums
}
