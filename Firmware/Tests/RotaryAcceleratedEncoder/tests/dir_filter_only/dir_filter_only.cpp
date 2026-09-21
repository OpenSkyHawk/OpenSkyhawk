// RotaryAcceleratedEncoder — dir_filter_only test (#287)
//
// DIR mode: the filter applies but there is no speed-up — the DIR frame carries exactly ±1 and
// PanelBridge drops anything else. Fast detents still emit +1; a reverse blip is swallowed.
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

OS::RotaryAcceleratedEncoder gEnc(CTRL_ID, PinRef(PA0), PinRef(PA1), OS::EncoderStepsPerDetent::Four,
                                  OS::EncoderMode::Dir);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== RotaryAcceleratedEncoder dir_filter_only ===");
    gEnc.configure();
    CANProtocol::startLoopback();
    gEnc.debugSeed(0);

    cw(gEnc); cw(gEnc);                                         // second one is "fast" timing-wise
    check("DIR detents are +1 each, never a step", gEnc.emitCount() == 2 && gEnc.lastValue() == 1 &&
                                                  gEnc.lastFrame() == canIdEvtDir(NODE_ID));
    // Reverse blip mid-spin (0→2, dropped), then the spin continues CW to the next detent (ends at 2).
    gEnc.debugStep(2); gEnc.debugStep(0); gEnc.debugStep(1); gEnc.debugStep(3); gEnc.debugStep(2);
    check("reverse blip swallowed in DIR too", gEnc.emitCount() == 3 && gEnc.netDetents() == 3 &&
                                              gEnc.lastValue() == 1);

    delay(OS::RotaryEncoder::STOPPED_THRESHOLD_MS + 100);
    gEnc.debugStep(2);                                          // idle sample → momentum reset
    gEnc.debugStep(3); gEnc.debugStep(1); gEnc.debugStep(0); gEnc.debugStep(2);   // CCW detent from 2
    check("reversal from rest -> -1", gEnc.emitCount() == 4 && gEnc.lastValue() == -1);

    CANProtocol::flushBatched(canIdEvtDir(NODE_ID));
    STM32Board::diagSerial().println(gPass ? "=== ALL PASS ===" : "=== FAIL ===");
}
void loop() {}
