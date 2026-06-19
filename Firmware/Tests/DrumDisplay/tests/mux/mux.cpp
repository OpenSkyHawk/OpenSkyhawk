// DrumDisplay — two panels on one TCA9548A I2cMux, driven at once.
//
// Two same-address (0x3C) SH1106 panels behind a TCA9548A at 0x70, on channels 0 and 1. Each
// DrumDisplay re-selects its channel before every buffer send, so two identically-addressed panels
// show two independent readouts. This is the several-at-once bench scenario (the mux + extra OLEDs).
//
// Tier A (logic): the two instances keep independent decoded state. Tier B (bench): channel 0 shows
// speed, channel 1 shows the BDHI range, with no cross-talk.

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <STM32Board.h>
#include <Outputs/DrumDisplay/DrumDisplay.h>
#include <Helpers/I2cMux/I2cMux.h>
#include <A4EC_OutputIds.h>

using namespace OpenSkyhawk;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled0(U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled1(U8G2_R0, U8X8_PIN_NONE);

// Readout descriptors — defined in the sketch (panel wiring, like the PinRef map), not a global.
static const DrumSource SPEED_SRC[] = {
    { A_4E_C_APN153_SPEED_X00, A_4E_C_APN153_SPEED_X00_AM, 1, 2 },
    { A_4E_C_APN153_SPEED_0X0, A_4E_C_APN153_SPEED_0X0_AM, 1, 1 },
    { A_4E_C_APN153_SPEED_00X, A_4E_C_APN153_SPEED_00X_AM, 1, 0 },
};
static const DrumReadout APN153_SPEED = {
    SPEED_SRC, 3, 3, 4.5f, 8.0f, 1.0f, 0.0f, 0, nullptr, 0,
    { false, 0, 0, nullptr, 0, 0.0f }, DrumScroll::SNAP_SETTLE, 3.0f,
};
static const DrumSource BDHI_DME_SRC[] = {
    { A_4E_C_BDHI_DME_X00, A_4E_C_BDHI_DME_X00_AM, 1, 2 },
    { A_4E_C_BDHI_DME_0X0, A_4E_C_BDHI_DME_0X0_AM, 1, 1 },
    { A_4E_C_BDHI_DME_00X, A_4E_C_BDHI_DME_00X_AM, 1, 0 },
};
static const DrumReadout BDHI_DME = {
    BDHI_DME_SRC, 3, 3, 4.5f, 8.0f, 1.0f, 0.0f, 0, nullptr, 0,
    { true, A_4E_C_BDHI_DME_FLAG, A_4E_C_BDHI_DME_FLAG_AM, " M", 3, 5.5f },
    DrumScroll::SNAP_SETTLE, 3.0f,
};

I2cMux mux(0x70, Wire);
DrumDisplay disp0(oled0, APN153_SPEED, mux, /*channel*/ 0, DrumFont::LARGE);
DrumDisplay disp1(oled1, BDHI_DME,     mux, /*channel*/ 1, DrumFont::LARGE);

static bool pass = true;
static void check(const char* label, bool ok) {
    if (!ok) pass = false;
    auto& d = STM32Board::diagSerial();
    d.print(ok ? F("[PASS] ") : F("[FAIL] "));
    d.println(label);
}
static uint16_t digitWord(int digit) {
    return static_cast<uint16_t>(lroundf(digit / 9.0f * 65535.0f));
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== DrumDisplay mux ==="));

    Wire.setSCL(PB8);
    Wire.setSDA(PB9);
    Wire.begin();
    oled0.setI2CAddress(0x3C << 1);
    oled1.setI2CAddress(0x3C << 1);
    disp0.configure();   // selects mux channel 0, then begin-blank
    disp1.configure();   // selects mux channel 1
    oled0.begin();
    oled1.begin();

    // Channel 0 = speed 250, channel 1 = BDHI range 045 — independent state.
    disp0.onControlPacket(A_4E_C_APN153_SPEED_X00, digitWord(2));
    disp0.onControlPacket(A_4E_C_APN153_SPEED_0X0, digitWord(5));
    disp0.onControlPacket(A_4E_C_APN153_SPEED_00X, digitWord(0));
    disp1.onControlPacket(A_4E_C_BDHI_DME_X00, digitWord(0));
    disp1.onControlPacket(A_4E_C_BDHI_DME_0X0, digitWord(4));
    disp1.onControlPacket(A_4E_C_BDHI_DME_00X, digitWord(5));

    check("channel 0 target == 250", disp0.debugTarget() == 250);
    check("channel 1 target == 45", disp1.debugTarget() == 45);
    check("independent state (no cross-write)",
          disp0.debugTarget() != disp1.debugTarget());

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {
    disp0.update();
    disp1.update();
}
