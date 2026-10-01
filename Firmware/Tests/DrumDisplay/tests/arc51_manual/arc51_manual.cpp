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
// share one address (mask-separated), so one packet updates two fields.
//
// These are defineMultipositionSwitch controls, so each field holds the selector's POSITION INDEX
// (Module:defineTumb exports n = round((arg − min)/step), clamped), not a normalised value — the
// Arduino library reads the same way, (data & mask) >> shift. The UHF display is XXX.XX:
//
//   10 MHz  18 positions → displayed 22..39, so two digits offset by 22
//    1 MHz  10 positions → one digit
//   50 kHz  20 positions → displayed 00,05..95, so two digits of 5
static const DrumSource ARC51_MANUAL_SRC[] = {
    { A_4E_C_ARC51_FREQ_10MHZ, A_4E_C_ARC51_FREQ_10MHZ_AM, 2, 3, /*steps*/ 18, /*mul*/ 1, /*offset*/ 22 },
    { A_4E_C_ARC51_FREQ_1MHZ,  A_4E_C_ARC51_FREQ_1MHZ_AM,  1, 2, /*steps*/ 10 },
    { A_4E_C_ARC51_FREQ_50KHZ, A_4E_C_ARC51_FREQ_50KHZ_AM, 2, 0, /*steps*/ 20, /*mul*/ 5 },
};
static const DrumGlyph ARC51_DOT[] = { { '.', 3, 1.8f } };
static const DrumReadout ARC51_FREQ_MANUAL = {
    .sources = ARC51_MANUAL_SRC, .nSources = 3, .nDigits = 5,
    .digitWidthMm = 4.5f, .digitHeightMm = 8.0f, .interDigitGapMm = 1.0f,
    .glyphs = ARC51_DOT, .nGlyphs = 1,
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

    Wire.setSCL(I2C_TEST_SCL);
    Wire.setSDA(I2C_TEST_SDA);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();
    freq.configure();

    // The exact state a sim capture caught: 10 MHz at position 7, 1 MHz at 9, 50 kHz at 18 — which
    // the ARC51_FREQ_* float drums reported as 22937 / 58981 / 62085, i.e. 299.90 on the panel.
    // One packet carries both selectors on the shared address, mask-separated.
    const uint16_t shared = static_cast<uint16_t>((7u << 5) | (9u << 10));
    freq.onControlPacket(A_4E_C_ARC51_FREQ_10MHZ, shared);
    freq.onControlPacket(A_4E_C_ARC51_FREQ_50KHZ, 18);

    check("one packet fills both MHz fields", freq.debugTarget() / 100 == 299);
    check("selectors reconstruct 299.90    ", freq.debugTarget() == 29990);
    check("6 visual cells (5 digits + '.')", freq.debugCellCount() == 6);
    check("row fits <= 128 px", freq.debugRowWidth() <= 128);

    // Bottom and top of each selector's travel. Both MHz fields ride the SAME word, so each
    // packet has to carry both — sending one alone clears the other, which is the shared-address
    // behaviour this readout depends on.
    freq.onControlPacket(A_4E_C_ARC51_FREQ_10MHZ, static_cast<uint16_t>((0u << 5) | (9u << 10)));
    freq.onControlPacket(A_4E_C_ARC51_FREQ_50KHZ, 0);
    check("position 0 → 229.00             ", freq.debugTarget() == 22900);
    freq.onControlPacket(A_4E_C_ARC51_FREQ_10MHZ, static_cast<uint16_t>((17u << 5) | (9u << 10)));
    freq.onControlPacket(A_4E_C_ARC51_FREQ_50KHZ, 19);
    check("top positions → 399.95          ", freq.debugTarget() == 39995);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {
    freq.update();
}
