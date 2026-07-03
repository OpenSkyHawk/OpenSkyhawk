// ShiftBus — encoder_srpins test (chip-less: SHIFTBUS_TEST bypass)
//
// RotaryEncoder with A/B on '165 pins, loop-rate decode (no ISR): Gray states are injected
// into the bus frame and walked through transfer() + poll() — the real production read path
// (PinRef::read → cached '165 bit → decode → drain → EVT). Confirms detents fire with the
// right value/frame in both REL and DIR mode through SR pins, and that forceReport reseeds
// without emitting.
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No shift registers needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>

static constexpr uint16_t CTRL_REL = 0x1111;
static constexpr uint16_t CTRL_DIR = 0x2222;

// A on D1, B on D0 of '165 chip 0 → injected Gray state = (A<<1)|B = the frame low bits.
OpenSkyhawk::RotaryEncoder gRel(CTRL_REL, PinRef(ShiftBus1, 0, 1), PinRef(ShiftBus1, 0, 0),
                                OpenSkyhawk::EncoderStepsPerDetent::Four,
                                OpenSkyhawk::EncoderMode::Rel);
// A on D3, B on D2.
OpenSkyhawk::RotaryEncoder gDir(CTRL_DIR, PinRef(ShiftBus1, 0, 3), PinRef(ShiftBus1, 0, 2),
                                OpenSkyhawk::EncoderStepsPerDetent::Four,
                                OpenSkyhawk::EncoderMode::Dir);

static void injectStates(uint8_t relState, uint8_t dirState) {
    ShiftBus1.testInjectIn(0, (uint8_t)((relState & 0x3) | ((dirState & 0x3) << 2)));
    ShiftBus1.transfer();
    gRel.poll();
    gDir.poll();
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println("=== ShiftBus encoder_srpins ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        d.print(label);
        d.println(ok ? ": PASS" : ": FAIL");
    };

    ShiftBus1.testSetBypass(true);
    gRel.configure();
    gDir.configure();
    ShiftBus1.begin();
    CANProtocol::start();

    // Seed both at state 0 through the real path (forceReport reads the injected frame).
    ShiftBus1.testInjectIn(0, 0x00);
    ShiftBus1.transfer();
    gRel.forceReport();
    gDir.forceReport();
    check("forceReport emits nothing", gRel.emitCount() == 0 && gDir.emitCount() == 0);

    // One CW detent on both simultaneously: 0→1→3→2→0 (4 transitions = 1 detent @Four).
    const uint8_t cw[] = {1, 3, 2, 0};
    for (uint8_t s : cw) injectStates(s, s);
    check("REL CW detent through SR pins -> +step on rel frame",
          gRel.emitCount() == 1 && gRel.lastValue() == OpenSkyhawk::RotaryEncoder::DEFAULT_STEP
          && gRel.lastFrame() == canIdEvtRel(NODE_ID));
    check("DIR CW detent through SR pins -> +1 on dir frame",
          gDir.emitCount() == 1 && gDir.lastValue() == 1
          && gDir.lastFrame() == canIdEvtDir(NODE_ID));

    // One CCW detent: 0→2→3→1→0.
    const uint8_t ccw[] = {2, 3, 1, 0};
    for (uint8_t s : ccw) injectStates(s, s);
    check("REL CCW -> -step", gRel.emitCount() == 2
          && gRel.lastValue() == -OpenSkyhawk::RotaryEncoder::DEFAULT_STEP);
    check("DIR CCW -> -1", gDir.emitCount() == 2 && gDir.lastValue() == -1);

    CANProtocol::flushBatched(canIdEvtRel(NODE_ID));
    CANProtocol::flushBatched(canIdEvtDir(NODE_ID));
    d.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
