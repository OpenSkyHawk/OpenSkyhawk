// DrumDisplay — APN-153 ground speed (3-digit) bring-up + decode test.
//
// 1.3" SH1106 @ 0x3C on STM32F103C8 (Blue Pill). I2C1 remap PB8 = SCL / PB9 = SDA (matches the
// renderer prototype). If the panel is an SSD1306, swap the U8G2 constructor below.
//
// Tier A (logic — no panel needed): decode/splice reconstructs the combined target; a foreign
// controlId is ignored. Tier B (bench eyeball): the tape rolls and snaps on the OLED.

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <STM32Board.h>
#include <DrumDisplay.h>
#include <A4EC_OutputIds.h>

using namespace OpenSkyhawk;

// Full-buffer hardware-I2C SH1106. SSD1306 alternative on the line below.
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// Readout descriptor — defined in the sketch (panel wiring, like the PinRef map), not a global.
static const DrumSource SPEED_SRC[] = {
    { A_4E_C_APN153_SPEED_X00, A_4E_C_APN153_SPEED_X00_AM, 1, 2 },
    { A_4E_C_APN153_SPEED_0X0, A_4E_C_APN153_SPEED_0X0_AM, 1, 1 },
    { A_4E_C_APN153_SPEED_00X, A_4E_C_APN153_SPEED_00X_AM, 1, 0 },
};
static const DrumReadout APN153_SPEED = {
    .sources = SPEED_SRC, .nSources = 3, .nDigits = 3,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
};

DrumDisplay speed(oled, APN153_SPEED, DrumFont::LARGE);

static bool pass = true;
static void check(const char* label, bool ok) {
    if (!ok) pass = false;
    auto& d = STM32Board::diagSerial();
    d.print(ok ? F("[PASS] ") : F("[FAIL] "));
    d.println(label);
}

// A full-word source carries 0..65535 for one digit 0..9: value = round(digit / 9 * 65535).
static uint16_t digitWord(int digit) {
    return static_cast<uint16_t>(lroundf(digit / 9.0f * 65535.0f));
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== DrumDisplay speed ==="));

    Wire.setSCL(PB8);
    Wire.setSDA(PB9);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();
    speed.configure();

    // Spell 250 across the three sources.
    speed.onControlPacket(A_4E_C_APN153_SPEED_X00, digitWord(2));
    speed.onControlPacket(A_4E_C_APN153_SPEED_0X0, digitWord(5));
    speed.onControlPacket(A_4E_C_APN153_SPEED_00X, digitWord(0));
    check("target reconstructs to 250", speed.debugTarget() == 250);
    check("3 visual cells (no glyph/flag)", speed.debugCellCount() == 3);
    check("row fits the 128 px panel", speed.debugRowWidth() <= 128);

    // A foreign controlId must not touch this readout.
    speed.onControlPacket(0x0000, 0xFFFF);
    check("foreign id ignored", speed.debugTarget() == 250);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {
    speed.update();  // bench: drives the rolling animation on the panel
}
