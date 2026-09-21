// RotaryEncoder — partial_no_emit test
//
// At OpenSkyhawk::EncoderStepsPerDetent::Four, movement that does not complete a detent (here 2 of 4 transitions)
// accumulates delta but emits nothing — the click only fires on a full detent.
//
// Rig: this STM32 alone, CAN in silent loopback (no bus, no PanelBridge, no encoder hardware).
// Do NOT run it on a board wired to a live bus: a loopback node never ACKs, which drives the
// bridge error-passive. PASS/FAIL comes from the encoder's own test seams, not from CAN.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>

static constexpr uint16_t CTRL_ID = 0x567B;

OpenSkyhawk::RotaryEncoder gEnc(CTRL_ID, PinRef(PA0), PinRef(PA1), OpenSkyhawk::EncoderStepsPerDetent::Four);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== RotaryEncoder partial_no_emit ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gEnc.configure();
    CANProtocol::startLoopback();

    gEnc.debugSeed(0);
    gEnc.debugStep(1); gEnc.debugStep(3);   // 2 of 4 transitions → delta 2 < 4
    check("partial (2/4): no EVT", gEnc.emitCount() == 0);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
