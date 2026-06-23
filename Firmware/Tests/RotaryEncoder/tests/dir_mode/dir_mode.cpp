// RotaryEncoder — dir_mode test
//
// DIR mode (fixed_step selector with no indicator, e.g. ARC-51 freq): each detent emits a signed
// ±1 DIRECTION on the DIR frame (canIdEvtDir), which the bridge turns into INC/DEC. Confirms the
// value (±1, not ±step) and the frame routing distinct from REL. OpenSkyhawk::EncoderStepsPerDetent::Four.
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No encoder hardware needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>

static constexpr uint16_t CTRL_ID = 0x567B;

OpenSkyhawk::RotaryEncoder gEnc(CTRL_ID, PinRef(PA0), PinRef(PA1),
                                OpenSkyhawk::EncoderStepsPerDetent::Four, OpenSkyhawk::EncoderMode::Dir);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== RotaryEncoder dir_mode ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gEnc.configure();
    CANProtocol::start();

    gEnc.debugSeed(0);
    gEnc.debugStep(1); gEnc.debugStep(3); gEnc.debugStep(2); gEnc.debugStep(0);   // CW detent
    check("DIR CW detent -> +1 on dir frame",
          gEnc.emitCount() == 1 && gEnc.lastValue() == 1 && gEnc.lastFrame() == canIdEvtDir(NODE_ID));

    gEnc.debugStep(2); gEnc.debugStep(3); gEnc.debugStep(1); gEnc.debugStep(0);   // CCW detent
    check("DIR CCW detent -> -1", gEnc.emitCount() == 2 && gEnc.lastValue() == -1);

    CANProtocol::flushBatched(canIdEvtDir(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
