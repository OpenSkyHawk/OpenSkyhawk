// DrumDisplay — current longitude (6-digit + E/W hemisphere flag) auto-fit test.
//
// SH1106 1.3" @ 0x3C, STM32F103C8. Exercises the widest readout: 6 digits + a flag cell, which
// overflows the 1.3" active width, so fitGeometry must auto-shrink the row to <= 128 px.
//
// Tier A (logic): the 6 sources reconstruct the combined number; the laid-out row fits the panel
// (auto-shrink fired). Tier B (bench): all 6 digits + the E/W flag read on the panel.

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <STM32Board.h>
#include <DrumDisplay.h>
#include <A4EC_OutputIds.h>

using namespace OpenSkyhawk;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// Readout descriptor — defined in the sketch (panel wiring, like the PinRef map), not a global.
static const DrumSource LON_SRC[] = {
    { A_4E_C_NAV_CURPOS_LON_X00000, A_4E_C_NAV_CURPOS_LON_X00000_AM, 1, 5 },
    { A_4E_C_NAV_CURPOS_LON_0X0000, A_4E_C_NAV_CURPOS_LON_0X0000_AM, 1, 4 },
    { A_4E_C_NAV_CURPOS_LON_00X000, A_4E_C_NAV_CURPOS_LON_00X000_AM, 1, 3 },
    { A_4E_C_NAV_CURPOS_LON_000X00, A_4E_C_NAV_CURPOS_LON_000X00_AM, 1, 2 },
    { A_4E_C_NAV_CURPOS_LON_0000X0, A_4E_C_NAV_CURPOS_LON_0000X0_AM, 1, 1 },
    { A_4E_C_NAV_CURPOS_LON_00000X, A_4E_C_NAV_CURPOS_LON_00000X_AM, 1, 0 },
};
// TODO(bench): rightmost source carries the ones digit and/or the E/W hemisphere — confirm.
static const DrumReadout NAV_CURPOS_LON = {
    .sources = LON_SRC, .nSources = 6, .nDigits = 6,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
    .flag = { .enabled = true, .address = A_4E_C_NAV_CURPOS_LON_00000X, .mask = A_4E_C_NAV_CURPOS_LON_00000X_AM, .faces = "EW", .atVisualCol = 6, .widthMm = 5.5f },
};

DrumDisplay lon(oled, NAV_CURPOS_LON, DrumFont::LARGE);

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
    d.println(F("=== DrumDisplay nav_pos (LON) ==="));

    Wire.setSCL(PB8);
    Wire.setSDA(PB9);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();
    lon.configure();

    // Spell 123456 across the six longitude sources (rightmost also drives the E/W flag).
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_X00000, digitWord(1));
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_0X0000, digitWord(2));
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_00X000, digitWord(3));
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_000X00, digitWord(4));
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_0000X0, digitWord(5));
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_00000X, digitWord(6));

    check("target reconstructs to 123456", lon.debugTarget() == 123456);
    check("7 visual cells (6 digits + flag)", lon.debugCellCount() == 7);
    check("auto-shrink fits row to <= 128 px", lon.debugRowWidth() <= 128);
    check("flag decoded to face index 1 (W)", lon.debugFlagTarget() == 1);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {
    lon.update();
}
