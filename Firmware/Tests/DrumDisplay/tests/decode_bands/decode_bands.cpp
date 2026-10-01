// DrumDisplay — band decode against real exported values (#137)
//
// The old decode scaled a source as value/mask × (10^nDigits − 1), i.e. it assumed a digit drum
// spanned its range in 9 steps. It does not. Verified through the DCS-BIOS and mod sources, and
// against two recorded sim captures:
//
//   utils.lua jumpwheel()        → a digit drum exports B/10 (and (B+dd)/10 while rolling)
//   Module.valueConvert()        → the gauge's declared arg range mapped linearly onto 0..65535
//   MemoryAllocation:setValue()  → math.floor, so a clean digit lands ONE LSB below its band edge
//   Module:defineTumb()          → a selector exports its POSITION INDEX, in a packed field
//
// So digit 9 arrives as 58981, which the old scale rendered as 8 — every digit from 5 up read one
// low. The values below are the ones the captures actually carry.
//
// Rig: this STM32 alone. Decode is pure arithmetic in onControlPacket(); no panel is read.

#include <Arduino.h>
#include <Wire.h>
#include <STM32Board.h>
#include <DrumDisplay.h>
#include <A4EC_OutputIds.h>

using namespace OpenSkyhawk;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// APN-153 ground speed: three plain digit drums, the default band (10 steps).
static const DrumSource SPEED_SRC[] = {
    { A_4E_C_APN153_SPEED_X00, A_4E_C_APN153_SPEED_X00_AM, 1, 2 },
    { A_4E_C_APN153_SPEED_0X0, A_4E_C_APN153_SPEED_0X0_AM, 1, 1 },
    { A_4E_C_APN153_SPEED_00X, A_4E_C_APN153_SPEED_00X_AM, 1, 0 },
};
static const DrumReadout SPEED = {
    .sources = SPEED_SRC, .nSources = 3, .nDigits = 3,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
};

DrumDisplay speed(oled, SPEED, DrumFont::LARGE);

static bool pass = true;
static void check(const char* label, bool ok) {
    if (!ok) pass = false;
    auto& d = STM32Board::diagSerial();
    d.print(ok ? F("[PASS] ") : F("[FAIL] "));
    d.println(label);
}

// Exactly what DCS puts on the wire for a clean digit: floor(d/10 × 65535).
static const uint16_t DIGIT_WORD[10] = {
    0, 6553, 13107, 19660, 26214, 32767, 39321, 45874, 52428, 58981,
};

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== DrumDisplay decode_bands ==="));

    Wire.setSCL(I2C_TEST_SCL);
    Wire.setSDA(I2C_TEST_SDA);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();
    speed.configure();

    // Every digit 0..9 must come back as itself. Under the old /9 scale, 5..9 each read one low.
    bool allDigits = true;
    for (int digit = 0; digit <= 9; digit++) {
        speed.onControlPacket(A_4E_C_APN153_SPEED_00X, DIGIT_WORD[digit]);
        const long got = speed.debugTarget() % 10;
        if (got != digit) {
            allDigits = false;
            d.print(F("  digit "));  d.print(digit);
            d.print(F(" word "));    d.print(DIGIT_WORD[digit]);
            d.print(F(" decoded ")); d.println(got);
        }
    }
    check("digits 0..9 each decode to self ", allDigits);

    // The specific regression: a live 9 is 58981, one LSB below its band edge because DCS floors.
    speed.onControlPacket(A_4E_C_APN153_SPEED_00X, 58981);
    check("58981 decodes to 9, not 8      ", speed.debugTarget() % 10 == 9);

    // A full-scale word cannot escape the top band.
    speed.onControlPacket(A_4E_C_APN153_SPEED_00X, 65535);
    check("65535 clamps to 9              ", speed.debugTarget() % 10 == 9);

    // Mid-roll: the drum sits between digits and the band keeps the lower one, which is what the
    // cascade then eases away from. 9.5/10 of the range is still a 9.
    speed.onControlPacket(A_4E_C_APN153_SPEED_00X, static_cast<uint16_t>(9.5f / 10.0f * 65535.0f));
    check("mid-roll 9.5 stays in band 9   ", speed.debugTarget() % 10 == 9);

    // A whole captured readout: 250 kn spells 2,5,0 — the 5 is the digit the old scale broke.
    speed.onControlPacket(A_4E_C_APN153_SPEED_X00, DIGIT_WORD[2]);
    speed.onControlPacket(A_4E_C_APN153_SPEED_0X0, DIGIT_WORD[5]);
    speed.onControlPacket(A_4E_C_APN153_SPEED_00X, DIGIT_WORD[0]);
    check("speed reads 250, not 240       ", speed.debugTarget() == 250);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() { speed.update(); }
