// RotaryAcceleratedEncoder — fast_slow test (#287)
//
// A detent completing < FAST_THRESHOLD_MS after the previous one sends fastStep; otherwise step.
// The first detent after a resync is always slow (no previous detent to time against).
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No encoder hardware needed.
#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/RotaryAcceleratedEncoder/RotaryAcceleratedEncoder.h>

namespace OS = OpenSkyhawk;
static constexpr uint16_t CTRL_ID = 0x567B;
static bool gPass = true;
static void check(const char* label, bool ok) {
    if (!ok) gPass = false;
    STM32Board::diagSerial().print(label);
    STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
}
// One full detent at EncoderStepsPerDetent::Four, starting and ending at Gray state 0.
template <class E> static void cw(E& e)  { e.debugStep(1); e.debugStep(3); e.debugStep(2); e.debugStep(0); }
template <class E> static void ccw(E& e) { e.debugStep(2); e.debugStep(3); e.debugStep(1); e.debugStep(0); }

OS::RotaryAcceleratedEncoder gEnc(CTRL_ID, PinRef(PA0), PinRef(PA1), OS::EncoderStepsPerDetent::Four,
                                  /*step*/ 3200, /*fastStep*/ 12800);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== RotaryAcceleratedEncoder fast_slow ===");
    gEnc.configure();
    CANProtocol::start();
    gEnc.debugSeed(0);

    cw(gEnc);
    check("first detent after resync is slow", gEnc.lastValue() == 3200);
    cw(gEnc);
    check("detent < 175 ms later is fast", gEnc.lastValue() == 12800);
    delay(OS::RotaryEncoder::FAST_THRESHOLD_MS + 75);
    cw(gEnc);
    check("detent >= 175 ms later is slow", gEnc.lastValue() == 3200);
    check("REL frame, three emits", gEnc.emitCount() == 3 && gEnc.lastFrame() == canIdEvtRel(NODE_ID));

    CANProtocol::flushBatched(canIdEvtRel(NODE_ID));
    STM32Board::diagSerial().println(gPass ? "=== ALL PASS ===" : "=== FAIL ===");
}
void loop() {}
