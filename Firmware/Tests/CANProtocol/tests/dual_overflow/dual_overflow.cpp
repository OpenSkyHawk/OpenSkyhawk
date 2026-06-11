// CANProtocol — TX ring overflow drop count integration test
//
// Requires two boards on a physical CAN bus with 120 Ω termination at each end.
// Both boards can be powered simultaneously — tester waits 1 s before firing so
// the ACK node's CAN peripheral has time to start. Without an ACK-er on the bus
// the sender enters error state instead of overflowing the TX ring.
//
// NODE_ID=0 (tester):
//   Waits 1 s, then queues 30 canIdHb(0) frames with IRQs disabled so the
//   TX-complete ISR cannot drain the ring between calls. TX capacity with IRQs
//   off = 3 mailboxes + 15 ring slots = 18 frames (ring holds TX_RING_SIZE-1).
//   Frames 19-30 (12 frames) drop immediately with txDropCount() increment.
//   Reports at t=3 s: drops > 0 → PASS.
//
// NODE_ID=1 (ACK node):
//   Accepts all frames; ACKs on bus so tester completes transmissions cleanly.
//   Reports received count at t=4 s.
//   Conservation check: tester drops + ACK rx_count should equal 30.
//
// Flash:
//   pio run -e test_dual_overflow_tester -t upload   (Board A, NODE_ID=0)
//   pio run -e test_dual_overflow_ack    -t upload   (Board B, NODE_ID=1)
// Monitor: open DiagSerial (115200) on both boards.
// Both boards can be powered at the same time.

#include <STM32Board.h>
#include <CANProtocol.h>

#if NODE_ID == 0  // ── Tester ──────────────────────────────────────────────────

static uint32_t _dropsBefore = 0;
static bool     _burstSent   = false;

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== dual_overflow TESTER (NODE_ID=0) ===");
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::filterAcceptAll();
    CANProtocol::start();
    STM32Board::log("Waiting 1 s for ACK node to start CAN...");
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    if (!_burstSent && millis() > 1000) {
        _burstSent = true;
        // Disable IRQs so TX-complete ISR cannot drain the ring between send() calls.
        // Without this, fast ACKs free mailboxes mid-loop and the ring never fills.
        // First 3 frames → mailboxes, frames 4-18 → ring (cap=15), frames 19-30 → drop.
        __disable_irq();
        _dropsBefore = CANProtocol::txDropCount();
        for (uint8_t i = 0; i < 30; i++) {
            uint8_t buf[8] = {i};
            CANProtocol::send(canIdHb(0), buf, 8);
        }
        __enable_irq();
        STM32Board::log("[TESTER] 30-frame burst queued");
    }

    static bool _reported = false;
    if (!_reported && millis() > 3000) {
        _reported = true;
        uint32_t drops   = CANProtocol::txDropCount() - _dropsBefore;
        uint32_t sentOk  = 30 - drops;
        auto& d = STM32Board::diagSerial();
        d.print(F("[TESTER] drops="));  d.print(drops);
        d.print(F("  sent_ok="));       d.println(sentOk);
        d.print(F("[TESTER] Overflow handled: "));
        d.println(drops > 0 ? F("PASS") : F("FAIL (no overflow — bus drained before ring filled)"));
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
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onReceive(onRx);
    CANProtocol::filterAcceptAll();
    CANProtocol::start();
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    static bool _reported = false;
    if (!_reported && millis() > 4000) {
        _reported = true;
        auto& d = STM32Board::diagSerial();
        d.print(F("[ACK] rx_count="));  d.println(_rxCount);
        d.println(F("Check: tester_drops + rx_count should == 30"));
    }
}

#endif
