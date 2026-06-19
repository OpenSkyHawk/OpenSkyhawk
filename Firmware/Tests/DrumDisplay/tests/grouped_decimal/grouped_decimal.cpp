// DrumDisplay — altimeter Kollsman setting (grouped 2+1+1 digits + decimal glyph) test.
//
// SH1106 1.3" @ 0x3C, STM32F103C8. Exercises a multi-digit source (NN00 carries 2 digits) and a
// static '.' glyph cell. Tier A (logic): the 2-digit splice + single-digit splices land "2992";
// the glyph adds a non-rolling cell. Tier B (bench): reads "29.92" with the dot fixed.

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <STM32Board.h>
#include <Outputs/DrumDisplay.h>
#include <A4EC_DrumReadouts.h>

using namespace OpenSkyhawk;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
DrumDisplay alt(oled, ALT_ADJ, DrumFont::LARGE);

static bool pass = true;
static void check(const char* label, bool ok) {
    if (!ok) pass = false;
    auto& d = STM32Board::diagSerial();
    d.print(ok ? F("[PASS] ") : F("[FAIL] "));
    d.println(label);
}
// One digit 0..9 in a whole word.
static uint16_t digitWord(int digit) {
    return static_cast<uint16_t>(lroundf(digit / 9.0f * 65535.0f));
}
// Two digits 0..99 in a whole word.
static uint16_t word2(int v) {
    return static_cast<uint16_t>(lroundf(v / 99.0f * 65535.0f));
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== DrumDisplay grouped_decimal (ALT_ADJ) ==="));

    Wire.setSCL(PB8);
    Wire.setSDA(PB9);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();
    alt.configure();

    // 29.92 -> NN00=29, tens=9, ones=2  -> combined 2992.
    alt.onControlPacket(A_4E_C_ALT_ADJ_NN00, word2(29));
    alt.onControlPacket(A_4E_C_ALT_ADJ_00N0, digitWord(9));
    alt.onControlPacket(A_4E_C_ALT_ADJ_000N, digitWord(2));

    check("2-digit splice reconstructs 2992", alt.debugTarget() == 2992);
    check("5 visual cells (4 digits + '.' glyph)", alt.debugCellCount() == 5);
    check("row fits <= 128 px", alt.debugRowWidth() <= 128);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {
    alt.update();
}
