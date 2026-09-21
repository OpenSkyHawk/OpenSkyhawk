// RotaryAcceleratedEncoder — burst_coalesce test (#287)
//
// Speed is classified per detent at decode time, so a burst drained late (e.g. after an OLED
// flush stalled the loop) still carries each detent's own magnitude. Slow + fast detents drain as
// ONE REL frame whose value is their sum; a sum beyond int16 splits into frames that add up.
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

static void cwNoDrain() { gEnc.debugDecode(1); gEnc.debugDecode(3); gEnc.debugDecode(2); gEnc.debugDecode(0); }

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== RotaryAcceleratedEncoder burst_coalesce ===");
    gEnc.configure();
    CANProtocol::start();
    gEnc.debugSeed(0);

    cwNoDrain();                                                // slow (first)
    cwNoDrain();                                                // fast
    delay(OS::RotaryEncoder::FAST_THRESHOLD_MS + 75);
    cwNoDrain();                                                // slow
    gEnc.debugDrain();
    check("slow+fast+slow drain as one frame = 3200+12800+3200",
          gEnc.emitCount() == 1 && gEnc.lastValue() == 19200 && gEnc.netDetents() == 3);

    cwNoDrain(); cwNoDrain(); cwNoDrain();                      // fast, fast, fast = 38400
    gEnc.debugDrain();
    check("overflowing sum splits into frames that add up (32767 + 5633)",
          gEnc.emitCount() == 3 && gEnc.lastValue() == 5633 && gEnc.netDetents() == 6);

    CANProtocol::flushBatched(canIdEvtRel(NODE_ID));
    STM32Board::diagSerial().println(gPass ? "=== ALL PASS ===" : "=== FAIL ===");
}
void loop() {}
