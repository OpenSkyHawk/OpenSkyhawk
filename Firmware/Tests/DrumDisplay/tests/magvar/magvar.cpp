// DrumDisplay — magnetic variation (5-digit + E/W flag) test.
//
// SH1106 1.3" @ 0x3C, STM32F103C8. Tier A (logic): 5 sources reconstruct the number, flag cell
// present. Tier B (bench): 5 digits + E/W flag read on the panel.

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <STM32Board.h>
#include <DrumDisplay.h>
#include <A4EC_OutputIds.h>

using namespace OpenSkyhawk;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// Readout descriptor — defined in the sketch (panel wiring, like the PinRef map), not a global.
// MagVar is FOUR digit drums plus a hemisphere, and the rightmost output is the flag — same shape
// as LAT/LON but a different flag scale: nav.lua's asn41_draw_magvar() sets xxxxX to 1 for East or
// 0 for West, i.e. FULL scale (steps = 1), where the ASN-41 position flags use half scale.
// Faces are ordered by position: index 0 = W (0.0), index 1 = E (1.0).
static const DrumSource MAGVAR_SRC[] = {
    { A_4E_C_ASN41_MAGVAR_X0000, A_4E_C_ASN41_MAGVAR_X0000_AM, 1, 3 },
    { A_4E_C_ASN41_MAGVAR_0X000, A_4E_C_ASN41_MAGVAR_0X000_AM, 1, 2 },
    { A_4E_C_ASN41_MAGVAR_00X00, A_4E_C_ASN41_MAGVAR_00X00_AM, 1, 1 },
    { A_4E_C_ASN41_MAGVAR_000X0, A_4E_C_ASN41_MAGVAR_000X0_AM, 1, 0 },
};
static const DrumReadout ASN41_MAGVAR = {
    .sources = MAGVAR_SRC, .nSources = 4, .nDigits = 4,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
    .flag = { .enabled = true, .address = A_4E_C_ASN41_MAGVAR_0000X, .mask = A_4E_C_ASN41_MAGVAR_0000X_AM, .faces = "WE", .atVisualCol = 4, .widthMm = 5.5f, .steps = 1 },
};

DrumDisplay magvar(oled, ASN41_MAGVAR, DrumFont::LARGE);

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
    d.println(F("=== DrumDisplay magvar ==="));

    Wire.setSCL(I2C_TEST_SCL);
    Wire.setSDA(I2C_TEST_SDA);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();
    magvar.configure();

    // Spell 1234.
    magvar.onControlPacket(A_4E_C_ASN41_MAGVAR_X0000, digitWord(1));
    magvar.onControlPacket(A_4E_C_ASN41_MAGVAR_0X000, digitWord(2));
    magvar.onControlPacket(A_4E_C_ASN41_MAGVAR_00X00, digitWord(3));
    magvar.onControlPacket(A_4E_C_ASN41_MAGVAR_000X0, digitWord(4));
    magvar.onControlPacket(A_4E_C_ASN41_MAGVAR_0000X, 65535);   // East, full scale

    check("target reconstructs to 1234", magvar.debugTarget() == 1234);
    check("5 visual cells (4 digits + flag)", magvar.debugCellCount() == 5);
    check("row fits <= 128 px", magvar.debugRowWidth() <= 128);
    check("full-scale 1.0 selects E (face 1)", magvar.debugFlagTarget() == 1);
    magvar.onControlPacket(A_4E_C_ASN41_MAGVAR_0000X, 0);
    check("0.0 selects W (face 0)          ", magvar.debugFlagTarget() == 0);

    // Captured values: MAGVAR_000X0 = 58981 is a live 9 and must not read 8.
    magvar.onControlPacket(A_4E_C_ASN41_MAGVAR_000X0, 58981);
    check("captured 58981 → 9              ", magvar.debugTarget() == 1239);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {
    magvar.update();
}
