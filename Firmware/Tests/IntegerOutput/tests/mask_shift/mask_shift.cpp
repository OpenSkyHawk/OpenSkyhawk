// IntegerOutput — mask and shift test
//
// DCS-BIOS packs several fields into one 16-bit address, so the base decodes
// (value & mask) >> shift before the callback sees it — the same triple DcsBios::IntegerBuffer
// takes. Verifies the decode, and that bits outside the mask are ignored.
//
// Hardware: STM32 alone — logic only, no pins, no CAN.

#include <Arduino.h>
#include <STM32Board.h>
#include <Outputs/IntegerOutput/IntegerOutput.h>

static constexpr uint16_t CTRL_ID = 0x1234;

static uint16_t gFieldCalls = 0, gFieldLast = 0;
static uint16_t gWholeLast  = 0;
static void onField(uint16_t v) { gFieldCalls++; gFieldLast = v; }
static void onWhole(uint16_t v) { gWholeLast = v; }

// Middle nibble of the word: mask 0x0F00, shift 8.
OpenSkyhawk::IntegerOutput gField(CTRL_ID, onField, 0x0F00, 8);
// Default mask/shift — the whole word, for comparison.
OpenSkyhawk::IntegerOutput gWhole(CTRL_ID, onWhole);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== IntegerOutput mask_shift ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gField.onControlPacket(CTRL_ID, 0xAB34);
    gWhole.onControlPacket(CTRL_ID, 0xAB34);
    check("masked+shifted: 0xAB34 -> 0xB  ", gFieldLast == 0x000B);
    check("default mask: whole word       ", gWholeLast == 0xAB34);

    // Only the masked nibble matters: the rest of the word changed, the field did not.
    gField.onControlPacket(CTRL_ID, 0x12B9);
    check("other bits changed: no new call", gFieldCalls == 1);
    check("field value unchanged          ", gFieldLast == 0x000B);

    // The field itself changes.
    gField.onControlPacket(CTRL_ID, 0x0700);
    check("field 7: called                ", gFieldCalls == 2);
    check("field 7: value 0x7             ", gFieldLast == 0x0007);

    // A zero field is a value, and reaches the callback.
    gField.onControlPacket(CTRL_ID, 0xF0FF);
    check("field 0: called                ", gFieldCalls == 3);
    check("field 0: value 0               ", gFieldLast == 0x0000);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
