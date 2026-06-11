// CANProtocol — TX ring overflow drop count integration test
//
// Requires two boards on a physical CAN bus with 120 Ω termination at each end.
// The ACK node must be present and running before the tester fires its burst —
// without a receiver ACKing frames, the sender enters error state.
//
// NODE_ID=0 (tester):
//   Queues 30 canIdHb(0) frames in a tight loop immediately after start(). At
//   500 kbps each frame takes ~222 µs; the loop completes in ~30 µs — faster than
//   one frame can transmit. TX capacity = 3 mailboxes + 16 ring entries = 19 frames.
//   Frames 20-30 (at least 11) should drop immediately with txDropCount() increment.
//   Reports at t=2 s: drops > 0 → PASS.
//
// NODE_ID=1 (ACK node):
//   Accepts all frames; ACKs on bus so tester completes transmissions cleanly.
//   Reports received count at t=3 s.
//   Conservation check: tester drops + ACK rx_count should equal 30.
//
// Flash:
//   pio run -e test_dual_overflow_tester -t upload   (Board A, NODE_ID=0)
//   pio run -e test_dual_overflow_ack    -t upload   (Board B, NODE_ID=1)
// Monitor: open DiagSerial (115200) on both boards.
// Sequence: power Board B first (ACK node must be ready before tester fires burst).

#include <STM32Board.h>
#include <CANProtocol.h>

#if NODE_ID == 0  // ── Tester ──────────────────────────────────────────────────

static uint32_t _dropsBefore = 0;

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== dual_overflow TESTER (NODE_ID=0) ===");
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::filterAcceptAll();
    CANProtocol::start();

    // Record baseline then fire burst immediately — no delay.
    // canIdHb(0)=0x100 is non-coalesceable; each frame queues normally until ring full.
    _dropsBefore = CANProtocol::txDropCount();
    for (uint8_t i = 0; i < 30; i++) {
        uint8_t buf[8] = {i};
        CANProtocol::send(canIdHb(0), buf, 8);
    }
    STM32Board::log("[TESTER] 30-frame burst queued");
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    static bool _reported = false;
    if (!_reported && millis() > 2000) {
        _reported = true;
        uint32_t drops   = CANProtocol::txDropCount() - _dropsBefore;
        uint32_t sentOk  = 30 - drops;
        auto& d = STM32Board::diagSerial();
        d.print(F("[TESTER] drops="));  d.print(drops);
        d.print(F("  sent_ok="));       d.println(sentOk);
        d.print(F("[TESTER] Overflow handled: "));
        d.println(drops > 0 ? F("PASS") : F("FAIL (bus drained too fast — try adding delay on ACK side)"));
        d.println(F("Check: drops + ACK rx_count should == 30"));
    }
}

#else  // ── ACK node (NODE_ID=1) ─────────────────────────────────────────────

static uint32_t _rxCount = 0;

static void onRx(uint32_t canId, const uint8_t* data, uint8_t len) {
    _rxCount++;
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== dual_overflow ACK NODE (NODE_ID=1) ===");
    STM32Board::log("Power this board first, then power the tester.");
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onReceive(onRx);
    CANProtocol::filterAcceptAll();
    CANProtocol::start();
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    static bool _reported = false;
    if (!_reported && millis() > 3000) {
        _reported = true;
        auto& d = STM32Board::diagSerial();
        d.print(F("[ACK] rx_count="));  d.println(_rxCount);
        d.println(F("Check: tester_drops + rx_count should == 30"));
    }
}

#endif
