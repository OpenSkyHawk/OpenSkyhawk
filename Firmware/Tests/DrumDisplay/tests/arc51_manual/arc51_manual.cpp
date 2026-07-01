// DrumDisplay — ARC-51 manual frequency from the 10/1/50 selectors (bit-packed sources).
//
// SH1106 1.3" @ 0x3C, STM32F103C8. The 10 MHz and 1 MHz selectors share one address
// (mask-separated), so this exercises onControlPacket scanning ALL sources for one packet.
//
// NOTE: the selector field encoding is a bench-confirm TODO (see the descriptor below). This test
// only asserts the smoke-level invariants (cell count, target in range, shared-address handling);
// the exact digit mapping is verified on the bench, not here.

#include <Arduino.h>
#include <Wire.h>
#include <STM32Board.h>
#include <DrumDisplay.h>
#include <A4EC_OutputIds.h>

using namespace OpenSkyhawk;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// Readout descriptor — defined in the sketch (panel wiring), not a global. The 10/1 MHz selectors
// share one address (mask-separated). TODO(bench): the bit-packed selector encoding is assumed.
static const DrumSource ARC51_MANUAL_SRC[] = {
    { A_4E_C_ARC51_FREQ_10MHZ, A_4E_C_ARC51_FREQ_10MHZ_AM, 1, 2 },
    { A_4E_C_ARC51_FREQ_1MHZ,  A_4E_C_ARC51_FREQ_1MHZ_AM,  1, 1 },
    { A_4E_C_ARC51_FREQ_50KHZ, A_4E_C_ARC51_FREQ_50KHZ_AM, 1, 0 },
};
static const DrumReadout ARC51_FREQ_MANUAL = {
    .sources = ARC51_MANUAL_SRC, .nSources = 3, .nDigits = 3,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
};

DrumDisplay freq(oled, ARC51_FREQ_MANUAL, DrumFont::LARGE);

static bool pass = true;
static void check(const char* label, bool ok) {
    if (!ok) pass = false;
    auto& d = STM32Board::diagSerial();
    d.print(ok ? F("[PASS] ") : F("[FAIL] "));
    d.println(label);
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== DrumDisplay arc51_manual ==="));

    Wire.setSCL(PB8);
    Wire.setSDA(PB9);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();
    freq.configure();

    // One packet to the shared 10/1 MHz address must update BOTH the place-2 and place-1 fields.
    freq.onControlPacket(A_4E_C_ARC51_FREQ_10MHZ, 0x4000);
    freq.onControlPacket(A_4E_C_ARC51_FREQ_50KHZ, 0x4000);

    check("3 visual cells (no glyph/flag)", freq.debugCellCount() == 3);
    check("target in 3-digit range [0,999]", freq.debugTarget() >= 0 && freq.debugTarget() <= 999);
    check("row fits <= 128 px", freq.debugRowWidth() <= 128);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {
    freq.update();
}
