// DrumDisplay — font size + panel-size auto-fit test.
//
// Two panels on the same bus: a 128x64 SH1106 (LARGE) and a 128x32 SSD1306 (SMALL). Exercises the
// resolution-independent fit (the 32-px panel clamps the roll window) and runtime setFontSize().
//
// Tier A (logic): both fit 3 cells within 128 px; a runtime LARGE->SMALL swap re-fits without
// breaking the layout. Tier B (bench): SMALL legible on the 0.91", LARGE on the 1.3".

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <STM32Board.h>
#include <DrumDisplay.h>
#include <A4EC_OutputIds.h>

using namespace OpenSkyhawk;

U8G2_SH1106_128X64_NONAME_F_HW_I2C    oledBig(U8G2_R0, U8X8_PIN_NONE);
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C oledSmall(U8G2_R0, U8X8_PIN_NONE);

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

DrumDisplay big(oledBig, APN153_SPEED, DrumFont::LARGE);
DrumDisplay small(oledSmall, APN153_SPEED, DrumFont::SMALL);

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
static void drive(DrumDisplay& dd) {
    dd.onControlPacket(A_4E_C_APN153_SPEED_X00, digitWord(2));
    dd.onControlPacket(A_4E_C_APN153_SPEED_0X0, digitWord(5));
    dd.onControlPacket(A_4E_C_APN153_SPEED_00X, digitWord(0));
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== DrumDisplay font ==="));

    Wire.setSCL(PB8);
    Wire.setSDA(PB9);
    Wire.begin();
    oledBig.setI2CAddress(0x3C << 1);
    oledBig.begin();
    oledSmall.setI2CAddress(0x3C << 1);
    oledSmall.begin();
    big.configure();
    small.configure();
    drive(big);
    drive(small);

    check("LARGE panel: 3 cells", big.debugCellCount() == 3);
    check("SMALL 128x32 panel: 3 cells", small.debugCellCount() == 3);
    check("SMALL row fits <= 128 px", small.debugRowWidth() <= 128);

    // Runtime swap LARGE -> SMALL re-fits on the next update().
    big.setFontSize(DrumFont::SMALL);
    big.update();
    check("re-fit after setFontSize keeps 3 cells", big.debugCellCount() == 3);

    // Regression (bench-found): setOffset() on a SETTLED readout must still re-fit + re-render.
    // The update() idle-skip used to return early when settled && !dirty, swallowing the re-fit,
    // so a runtime registration change silently did nothing. mm offset → px via the panel scale.
    for (int i = 0; i < 120; i++) { small.update(); delay(20); }   // drive to settled
    int16_t x0 = small.debugCellX0();
    small.setOffset(3.0f, 0.0f);   // +3 mm right
    small.update();
    check("setOffset re-positions a settled readout", small.debugCellX0() > x0);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {
    big.update();
    small.update();
}
