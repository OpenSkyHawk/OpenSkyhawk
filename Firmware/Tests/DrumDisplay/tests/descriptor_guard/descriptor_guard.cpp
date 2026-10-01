// DrumDisplay — descriptor bounds guard (#137)
//
// _pos[6] and _cellX[MAX_CELLS] are fixed arrays, so a hand-authored descriptor that exceeds them
// writes past the end of the object. decodeDigits() already bounds a SOURCE to 10^nDigits − 1, but
// nothing checked that the source fits the readout it is spliced into, nor that nDigits itself
// stays within the tape array.
//
// configure() now validates and disables the readout instead of laying it out. Each bad descriptor
// below trips a different branch; the good one still configures, so the guard is proven to reject
// and to permit.
//
// Rig: this STM32 alone — logic only. No OLED is needed: validation runs before any drawing, and an
// absent panel simply trips the I2cHealth breaker.

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <STM32Board.h>
#include <DrumDisplay.h>

using namespace OpenSkyhawk;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// One source filling a 3-digit readout — the shape every real descriptor has.
static const DrumSource SRC_OK[] = {{ 0x1234, 0xFFFF, 3, 0 }};

// place 2 + nDigits 2 = 4 columns in a 3-digit readout: one column past the end.
static const DrumSource SRC_OVERRUN[] = {{ 0x1234, 0xFFFF, 2, 2 }};

static const DrumReadout GOOD = {
    SRC_OK, 1, 3, 10.0f, 14.0f, 1.0f,
};

static const DrumReadout SPLICE_OVERRUN = {
    SRC_OVERRUN, 1, 3, 10.0f, 14.0f, 1.0f,
};

// 7 digit tapes — one more than _pos[6] holds.
static const DrumReadout TOO_MANY_DIGITS = {
    SRC_OK, 1, 7, 10.0f, 14.0f, 1.0f,
};

DrumDisplay gGood(oled, GOOD);
DrumDisplay gOverrun(oled, SPLICE_OVERRUN);
DrumDisplay gTooMany(oled, TOO_MANY_DIGITS);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== DrumDisplay descriptor_guard ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    Wire.begin();
    oled.setI2CAddress(0x3C << 1);

    // Force the I2C breaker closed on all three BEFORE configure(). With no panel on the bus a
    // real probe fails, and the breaker then backs off ~2 s ignoring any later override — which
    // would leave the render assertions below passing merely because nothing can be drawn at all.
    gGood.debugForceProbe(1);
    gOverrun.debugForceProbe(1);
    gTooMany.debugForceProbe(1);

    gGood.configure();
    gOverrun.configure();
    gTooMany.configure();

    check("valid descriptor: accepted      ", gGood.debugDescriptorOk());
    check("place+nDigits past width: refused", !gOverrun.debugDescriptorOk());
    check("nDigits 7 (> _pos[6]): refused  ", !gTooMany.debugDescriptorOk());

    // A refused readout must lay nothing out — no cells, nothing to draw past.
    check("refused: no cells laid out      ", gOverrun.debugCellCount() == 0 && gTooMany.debugCellCount() == 0);

    // And it must stay inert when driven. gGood is the control: same forced probe, same packet,
    // and it does render — so a flat render count on the refused pair is the guard doing the work,
    // not an unreachable panel skipping the draw.
    const uint32_t goodBefore = gGood.debugRenderCount();
    gGood.onControlPacket(0x1234, 0x8000);
    gGood.update();
    check("accepted: packet does render    ", gGood.debugRenderCount() > goodBefore);

    // nDigits 7 is the dangerous one: update()'s ease loop would write _pos[6], past the array.
    const uint32_t overrunBefore = gOverrun.debugRenderCount();
    const uint32_t tooManyBefore = gTooMany.debugRenderCount();
    gOverrun.onControlPacket(0x1234, 0x8000);
    gTooMany.onControlPacket(0x1234, 0x8000);
    gOverrun.update();
    gTooMany.update();
    check("refused: packet renders nothing ", gOverrun.debugRenderCount() == overrunBefore &&
                                              gTooMany.debugRenderCount() == tooManyBefore);


    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
