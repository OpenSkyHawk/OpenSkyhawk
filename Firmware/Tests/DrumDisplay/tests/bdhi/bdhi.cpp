// DrumDisplay — BDHI DME range (3-digit) + a real 2-state DME flag source.
//
// SH1106 1.3" @ 0x3C, STM32F103C8. Unlike the NAV hemisphere (a digit/flag dual-role), the BDHI
// DME flag is its own dedicated address, so this exercises a flag driven independently of digits.
//
// Tier A (logic): 3 sources reconstruct the range; the flag is its own cell. Tier B (bench): the
// range counter + DME off-flag render on the panel.

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <STM32Board.h>
#include <Outputs/DrumDisplay.h>
#include <A4EC_DrumReadouts.h>

using namespace OpenSkyhawk;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
DrumDisplay bdhi(oled, BDHI_DME, DrumFont::LARGE);

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
    d.println(F("=== DrumDisplay bdhi ==="));

    Wire.setSCL(PB8);
    Wire.setSDA(PB9);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();
    bdhi.configure();

    // Range 045.
    bdhi.onControlPacket(A_4E_C_BDHI_DME_X00, digitWord(0));
    bdhi.onControlPacket(A_4E_C_BDHI_DME_0X0, digitWord(4));
    bdhi.onControlPacket(A_4E_C_BDHI_DME_00X, digitWord(5));
    bdhi.onControlPacket(A_4E_C_BDHI_DME_FLAG, 1);  // DME valid

    check("target reconstructs to 45", bdhi.debugTarget() == 45);
    check("4 visual cells (3 digits + flag)", bdhi.debugCellCount() == 4);
    check("row fits <= 128 px", bdhi.debugRowWidth() <= 128);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {
    bdhi.update();
}
