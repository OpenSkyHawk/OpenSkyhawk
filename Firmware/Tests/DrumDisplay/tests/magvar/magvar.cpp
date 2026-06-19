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
static const DrumSource MAGVAR_SRC[] = {
    { A_4E_C_ASN41_MAGVAR_X0000, A_4E_C_ASN41_MAGVAR_X0000_AM, 1, 4 },
    { A_4E_C_ASN41_MAGVAR_0X000, A_4E_C_ASN41_MAGVAR_0X000_AM, 1, 3 },
    { A_4E_C_ASN41_MAGVAR_00X00, A_4E_C_ASN41_MAGVAR_00X00_AM, 1, 2 },
    { A_4E_C_ASN41_MAGVAR_000X0, A_4E_C_ASN41_MAGVAR_000X0_AM, 1, 1 },
    { A_4E_C_ASN41_MAGVAR_0000X, A_4E_C_ASN41_MAGVAR_0000X_AM, 1, 0 },
};
// TODO(bench): rightmost source carries the ones digit and/or the E/W hemisphere — confirm.
static const DrumReadout ASN41_MAGVAR = {
    MAGVAR_SRC, 5, 5, 4.5f, 8.0f, 1.0f, 0.0f, 0, nullptr, 0,
    { true, A_4E_C_ASN41_MAGVAR_0000X, A_4E_C_ASN41_MAGVAR_0000X_AM, "EW", 5, 5.5f },
    DrumScroll::SNAP_SETTLE, 3.0f,
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
    return static_cast<uint16_t>(lroundf(digit / 9.0f * 65535.0f));
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== DrumDisplay magvar ==="));

    Wire.setSCL(PB8);
    Wire.setSDA(PB9);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();
    magvar.configure();

    // Spell 12345.
    magvar.onControlPacket(A_4E_C_ASN41_MAGVAR_X0000, digitWord(1));
    magvar.onControlPacket(A_4E_C_ASN41_MAGVAR_0X000, digitWord(2));
    magvar.onControlPacket(A_4E_C_ASN41_MAGVAR_00X00, digitWord(3));
    magvar.onControlPacket(A_4E_C_ASN41_MAGVAR_000X0, digitWord(4));
    magvar.onControlPacket(A_4E_C_ASN41_MAGVAR_0000X, digitWord(5));

    check("target reconstructs to 12345", magvar.debugTarget() == 12345);
    check("6 visual cells (5 digits + flag)", magvar.debugCellCount() == 6);
    check("row fits <= 128 px", magvar.debugRowWidth() <= 128);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {
    magvar.update();
}
