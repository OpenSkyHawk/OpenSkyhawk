// RotaryAcceleratedEncoder — reverse_blip test (#287)
//
// A noisy encoder produces a single reverse transition in the middle of a spin. The momentum
// filter drops it (and only decays the momentum), so the spin keeps its full detent count. The
// same Gray sequence through a plain RotaryEncoder loses a detent — that is the difference the
// filter exists to make. Speed-up off (fastStep 0) so only the filter is exercised.
//
// Rig: this STM32 alone, CAN in silent loopback (no bus, no PanelBridge, no encoder hardware).
// Do NOT run it on a board wired to a live bus: a loopback node never ACKs, which drives the
// bridge error-passive. PASS/FAIL comes from the encoder's own test seams, not from CAN.
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

OS::RotaryAcceleratedEncoder gAccel(CTRL_ID, PinRef(PA0), PinRef(PA1), OS::EncoderStepsPerDetent::Four,
                                    /*step*/ 3200, /*fastStep*/ 0);
OS::RotaryEncoder gPlain(CTRL_ID, PinRef(PA0), PinRef(PA1), OS::EncoderStepsPerDetent::Four);

// CW detent, then: reverse blip (0→2), back (2→0), and three more CW transitions (0→1→3→2).
template <class E> static void spinWithBlip(E& e) {
    cw(e);
    e.debugStep(2);                              // the blip — one CCW transition
    e.debugStep(0); e.debugStep(1); e.debugStep(3); e.debugStep(2);
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== RotaryAcceleratedEncoder reverse_blip ===");
    gAccel.configure(); gPlain.configure();
    CANProtocol::startLoopback();

    gAccel.debugSeed(0);
    spinWithBlip(gAccel);
    check("filter: blip swallowed, both CW detents land", gAccel.netDetents() == 2 && gAccel.emitCount() == 2);
    check("filter: no reverse emit", gAccel.lastValue() == 3200);

    gPlain.debugSeed(0);
    spinWithBlip(gPlain);
    check("plain encoder loses a detent to the same blip", gPlain.netDetents() == 1);

    CANProtocol::flushBatched(canIdEvtRel(NODE_ID));
    STM32Board::diagSerial().println(gPass ? "=== ALL PASS ===" : "=== FAIL ===");
}
void loop() {}
