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
// Longitude is XXX.YY plus a hemisphere: FIVE digit drums, and the rightmost output is the flag.
// nav.lua sets NAV_CURPOS_LON_nnnnnX to E_W, which is 0.0 for West or 0.5 for East — half scale,
// hence flag steps = 2. Faces are ordered by position, so index 0 is W and index 1 is E.
static const DrumSource LON_SRC[] = {
    { A_4E_C_NAV_CURPOS_LON_X00000, A_4E_C_NAV_CURPOS_LON_X00000_AM, 1, 4 },
    { A_4E_C_NAV_CURPOS_LON_0X0000, A_4E_C_NAV_CURPOS_LON_0X0000_AM, 1, 3 },
    { A_4E_C_NAV_CURPOS_LON_00X000, A_4E_C_NAV_CURPOS_LON_00X000_AM, 1, 2 },
    { A_4E_C_NAV_CURPOS_LON_000X00, A_4E_C_NAV_CURPOS_LON_000X00_AM, 1, 1 },
    { A_4E_C_NAV_CURPOS_LON_0000X0, A_4E_C_NAV_CURPOS_LON_0000X0_AM, 1, 0 },
};
static const DrumReadout NAV_CURPOS_LON = {
    .sources = LON_SRC, .nSources = 5, .nDigits = 5,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
    .flag = { .enabled = true, .address = A_4E_C_NAV_CURPOS_LON_00000X, .mask = A_4E_C_NAV_CURPOS_LON_00000X_AM, .faces = "WE", .atVisualCol = 5, .widthMm = 5.5f, .steps = 2 },
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
    // DCS exports a drum digit as digit/10 of the gauge's arg range (utils.lua jumpwheel()
    // returns B/10), and TRUNCATES on the way out — a 9 arrives as 58981, not 65535.
    return static_cast<uint16_t>(digit / 10.0f * 65535.0f);
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== DrumDisplay nav_pos (LON) ==="));

    Wire.setSCL(I2C_TEST_SCL);
    Wire.setSDA(I2C_TEST_SDA);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();
    lon.configure();

    // Spell 12345 across the five longitude drums.
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_X00000, digitWord(1));
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_0X0000, digitWord(2));
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_00X000, digitWord(3));
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_000X00, digitWord(4));
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_0000X0, digitWord(5));
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_00000X, 32767);   // E_W = 0.5 → East

    check("target reconstructs to 12345", lon.debugTarget() == 12345);
    check("6 visual cells (5 digits + flag)", lon.debugCellCount() == 6);
    check("auto-shrink fits row to <= 128 px", lon.debugRowWidth() <= 128);

    // The hemisphere arrives at HALF scale. Read as full scale it never leaves face 0, which is
    // what the readout did before the per-source band landed.
    check("half-scale 0.5 selects E (face 1)", lon.debugFlagTarget() == 1);
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_00000X, 0);       // E_W = 0.0 → West
    check("0.0 selects W (face 0)          ", lon.debugFlagTarget() == 0);

    // The hemisphere output must not move a digit: it is not part of the number.
    check("flag source is not a digit      ", lon.debugTarget() == 12345);

    // Values straight off a sim capture: a live 9 must read 9, not 8.
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_00X000, 58981);   // digit 9
    lon.onControlPacket(A_4E_C_NAV_CURPOS_LON_000X00, 32767);   // digit 5
    check("captured 58981/32767 → 9 and 5  ", lon.debugTarget() == 12955);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {
    lon.update();
}
