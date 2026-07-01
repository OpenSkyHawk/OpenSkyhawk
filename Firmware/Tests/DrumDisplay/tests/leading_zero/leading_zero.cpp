// DrumDisplay — leading-zero suppression (#196).
//
// Variable-width readout (the AN/ARC-51A preset channel: 1..20 shown "1".."20", never "01").
// suppressLeadingZero blanks the high-order cell until the value needs it; the default-false
// path keeps every cell (fixed width). Logic-level assertions — debugVisibleDigits() is the
// exact digit-cell count the render draws, so no OLED is needed: flash any bluepill and read
// the serial PASS/FAIL. (CI compiles all envs on genericSTM32F103C8.)

#include <Arduino.h>
#include <STM32Board.h>
#include <DrumDisplay.h>

using namespace OpenSkyhawk;

// U8G2 objects only satisfy the ctor; the test never begins I2C or renders (pure decode/logic).
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C oledA(U8G2_R0, U8X8_PIN_NONE);
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C oledB(U8G2_R0, U8X8_PIN_NONE);

// Two 1-digit whole-word sources (units @ place 0, tens @ place 1): value 0..65535 → digit 0..9,
// so a synthetic value sets each digit exactly, giving full control of the combined target.
static const uint16_t ADDR_UNITS = 0xF000;
static const uint16_t ADDR_TENS  = 0xF002;
static const DrumSource CH_SRC[] = {
    { ADDR_UNITS, 0xFFFF, 1, 0 },
    { ADDR_TENS,  0xFFFF, 1, 1 },
};
static uint16_t digitVal(int d) {  // value that decodes to single digit d (round(d/9 * 65535))
    return static_cast<uint16_t>(lroundf(static_cast<float>(d) / 9.0f * 65535.0f));
}

// suppress ON — the ARC-51 channel window
static const DrumReadout CH_SUPPRESS = {
    CH_SRC, 2, 2, 4.5f, 8.0f, 1.0f, 0.0f, 0, nullptr, 0,
    { false, 0, 0, nullptr, 0, 0.0f }, DrumScroll::SNAP_SETTLE, 3.0f, LeadingZero::Suppress,
};
// suppress OFF — control: default behaviour (leadingZero defaults to Keep, not specified)
static const DrumReadout CH_PLAIN = {
    CH_SRC, 2, 2, 4.5f, 8.0f, 1.0f, 0.0f, 0, nullptr, 0,
    { false, 0, 0, nullptr, 0, 0.0f }, DrumScroll::SNAP_SETTLE, 3.0f,
};

DrumDisplay chSup(oledA, CH_SUPPRESS, DrumFont::LARGE);
DrumDisplay chPln(oledB, CH_PLAIN, DrumFont::LARGE);

static bool pass = true;
static void check(const char* label, bool ok) {
    if (!ok) pass = false;
    auto& d = STM32Board::diagSerial();
    d.print(ok ? F("[PASS] ") : F("[FAIL] "));
    d.println(label);
}

static void setValue(DrumDisplay& dd, int tens, int units) {
    dd.onControlPacket(ADDR_TENS,  digitVal(tens));
    dd.onControlPacket(ADDR_UNITS, digitVal(units));
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== DrumDisplay leading_zero (#196) ==="));

    // suppress ON: 9 -> 10 -> 9 -> 0 (no OLED begin/configure needed — decode + logic only)
    setValue(chSup, 0, 9);
    check("suppress: 9 -> 1 digit",              chSup.debugTarget() == 9  && chSup.debugVisibleDigits() == 1);
    setValue(chSup, 1, 0);
    check("suppress: 10 -> 2 digits",            chSup.debugTarget() == 10 && chSup.debugVisibleDigits() == 2);
    setValue(chSup, 0, 9);
    check("suppress: back to 9 -> 1 digit",      chSup.debugTarget() == 9  && chSup.debugVisibleDigits() == 1);
    setValue(chSup, 0, 0);
    check("suppress: 0 -> 1 digit (never blank)", chSup.debugTarget() == 0  && chSup.debugVisibleDigits() == 1);

    // suppress OFF: default path unchanged — always nDigits, leading zero kept
    setValue(chPln, 0, 9);
    check("plain: 9 -> 2 digits (leading 0 kept)", chPln.debugTarget() == 9 && chPln.debugVisibleDigits() == 2);
    setValue(chPln, 0, 0);
    check("plain: 0 -> 2 digits",                  chPln.debugTarget() == 0 && chPln.debugVisibleDigits() == 2);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {}
