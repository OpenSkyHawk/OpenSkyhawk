// Dimmer — dedup test
//
// Two layers of dedup have to hold, because DCS-BIOS re-broadcasts full state after SYNC_REQ and
// analogWrite() reconfigures the timer channel on every call:
//   base  — the same decoded value never reaches apply() twice in a row
//   class — different values that land on the same 8-bit duty write once
// A genuinely different duty must still get through.
//
// Hardware: STM32 alone. PB0 (TIM3_CH3), nothing wired — assertions read the test seam.

#include <Arduino.h>
#include <STM32Board.h>
#include <Outputs/Dimmer/Dimmer.h>

static constexpr uint8_t  PIN_PWM = PB0;
static constexpr uint16_t CTRL_ID = 0x1234;

OpenSkyhawk::Dimmer gDim(CTRL_ID, PinRef(PIN_PWM));

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== Dimmer dedup ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gDim.configure();

    // Base dedup: the identical value repeated (a SYNC_REQ re-broadcast) writes once.
    gDim.onControlPacket(CTRL_ID, 0x8000);
    gDim.onControlPacket(CTRL_ID, 0x8000);
    gDim.onControlPacket(CTRL_ID, 0x8000);
    check("same value x3: one write        ", gDim.writeCount() == 1);
    check("same value x3: duty 128         ", gDim.lastDuty() == 128);

    // Class dedup: a different value whose top byte is unchanged is the same duty.
    gDim.onControlPacket(CTRL_ID, 0x80FF);
    check("0x80FF (same duty): no write    ", gDim.writeCount() == 1);
    check("0x80FF: duty still 128          ", gDim.lastDuty() == 128);

    // A real change still gets through.
    gDim.onControlPacket(CTRL_ID, 0x4000);
    check("0x4000: writes                  ", gDim.writeCount() == 2);
    check("0x4000: duty 64                 ", gDim.lastDuty() == 64);

    // And back again — no stuck state.
    gDim.onControlPacket(CTRL_ID, 0x8000);
    check("back to 0x8000: writes          ", gDim.writeCount() == 3);
    check("back to 0x8000: duty 128        ", gDim.lastDuty() == 128);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
