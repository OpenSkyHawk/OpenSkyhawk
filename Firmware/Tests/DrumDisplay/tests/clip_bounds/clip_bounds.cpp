// DrumDisplay — clip rect stays on the panel (#137)
//
// Found on a real panel during the visual tour: the offset stage rendered a BLANK screen. The clip
// rect is built from _cy and _cellH, and u8g2's coordinates are unsigned — so an edge that computes
// negative wraps to 65535, setClipWindow yields an empty window, and the readout disappears rather
// than being clipped. A setOffset() nudge is enough: at PX_PER_MM 4.35, a -2 mm y offset puts _cy
// at 23, and an 11 mm cell (48 px) makes the top edge 23 - 24 = -1.
//
// Each case below is a configuration the descriptor and the runtime setters are supposed to accept.
//
// Rig: this STM32 alone. Geometry is computed from the U8G2 buffer dimensions; no panel is read.

#include <Arduino.h>
#include <Wire.h>
#include <STM32Board.h>
#include <DrumDisplay.h>
#include <A4EC_OutputIds.h>

using namespace OpenSkyhawk;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

static const DrumSource SPEED_SRC[] = {
    { A_4E_C_APN153_SPEED_X00, A_4E_C_APN153_SPEED_X00_AM, 1, 2 },
    { A_4E_C_APN153_SPEED_0X0, A_4E_C_APN153_SPEED_0X0_AM, 1, 1 },
    { A_4E_C_APN153_SPEED_00X, A_4E_C_APN153_SPEED_00X_AM, 1, 0 },
};
// The geometry the tour used: 11 mm cells, so 48 px of a 64 px panel.
static const DrumReadout SPEED_BIG = {
    .sources = SPEED_SRC, .nSources = 3, .nDigits = 3,
    .digitWidthMm = 7.0f, .digitHeightMm = 11.0f, .interDigitGapMm = 1.5f,
};

DrumDisplay centred(oled, SPEED_BIG, DrumFont::LARGE);
DrumDisplay shifted(oled, SPEED_BIG, DrumFont::LARGE, /*xOffsetMm*/ 4.0f, /*yOffsetMm*/ -2.0f);
DrumDisplay runtime(oled, SPEED_BIG, DrumFont::LARGE);

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
    d.println(F("=== DrumDisplay clip_bounds ==="));

    Wire.setSCL(I2C_TEST_SCL);
    Wire.setSDA(I2C_TEST_SDA);
    Wire.begin();
    oled.setI2CAddress(0x3C << 1);
    oled.begin();

    centred.configure();
    shifted.configure();
    runtime.configure();

    check("centred readout clips on-panel", centred.debugClipFits());

    // The constructor offset that blanked the panel.
    check("ctor offset (+4,-2) clips on-panel", shifted.debugClipFits());

    // The same offset applied at runtime must behave identically.
    runtime.setOffset(4.0f, -2.0f);
    check("setOffset(+4,-2) clips on-panel ", runtime.debugClipFits());

    // Push it hard the other way: the row goes off the top, cells that survive stay valid.
    runtime.setOffset(-6.0f, 6.0f);
    check("setOffset(-6,+6) clips on-panel ", runtime.debugClipFits());

    // And back to centre.
    runtime.setOffset(0.0f, 0.0f);
    check("offset cleared, still on-panel  ", runtime.debugClipFits());

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {}
