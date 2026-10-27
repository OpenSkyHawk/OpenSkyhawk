// CANProtocol — CTRL_BCAST burst order integration test (#389)
//
// Requires two boards on a physical CAN bus with 120 Ω termination at each end.
//
// NODE_ID=0 (sender):
//   Waits 2 s after start() for the receiver to boot and configure its filter, then
//   queues 20 CTRL_BCAST frames with data[0]=1..20 in a tight loop. The loop runs
//   faster than the bus can drain frames (~1 µs/iteration vs ~222 µs/frame at 500 kbps),
//   so the frames wait in the TX ring. Nothing is coalesced (#389): a frame carries two of
//   ~250 controls, so every one must arrive. Reports txDropCount() at t=4 s.
//   Expected: drops=0 (the bridge's ring holds 64 frames).
//
// NODE_ID=1 (receiver):
//   Counts received CTRL_BCAST frames and checks each data[0] is the previous + 1.
//   Reports at t=5 s (1 s after sender finishes).
//   PASS: rxCount == 20, every frame in order, lastVal == 20.
//
// Flash:
//   pio run -e test_dual_order_sender   -t upload   (Board A, NODE_ID=0)
//   pio run -e test_dual_order_receiver -t upload   (Board B, NODE_ID=1)
// Monitor: open DiagSerial (115200) on both boards.

#include <STM32Board.h>
#include <CANProtocol.h>

#if NODE_ID == 0  // ── Sender ──────────────────────────────────────────────────

static bool _burstSent = false;

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== dual_order SENDER (NODE_ID=0) ===");
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::filterAcceptAll();
    CANProtocol::start();
    STM32Board::log("Waiting 2 s for receiver to boot...");
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    if (!_burstSent && millis() > 2000) {
        _burstSent = true;
        for (uint8_t i = 1; i <= 20; i++) {
            uint8_t buf[8] = {};
            buf[0] = i;
            CANProtocol::send(CAN_ID_CTRL_BCAST, buf, 8);
        }
        STM32Board::log("[SENDER] 20-frame burst queued");
    }

    static bool _reported = false;
    if (!_reported && millis() > 4000) {
        _reported = true;
        uint32_t drops = CANProtocol::txDropCount();
        auto& d = STM32Board::diagSerial();
        d.print(F("[SENDER] drops="));
        d.print(drops);
        d.println(drops == 0 ? F("  PASS") : F("  FAIL"));
    }
}

#else  // ── Receiver (NODE_ID=1) ─────────────────────────────────────────────

static uint32_t _rxCount = 0;
static uint8_t  _lastVal = 0;
static bool     _inOrder = true;

static void onRx(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId == CAN_ID_CTRL_BCAST && len >= 1) {
        _rxCount++;
        if (data[0] != (uint8_t)(_lastVal + 1)) _inOrder = false;
        _lastVal = data[0];
    }
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== dual_order RECEIVER (NODE_ID=1) ===");
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onReceive(onRx);
    CANProtocol::filterAcceptAll();
    CANProtocol::start();
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    static bool _reported = false;
    if (!_reported && millis() > 5000) {
        _reported = true;
        bool all = (_rxCount == 20 && _lastVal == 20);
        auto& d = STM32Board::diagSerial();
        d.print(F("[RECV] rx_count=")); d.print(_rxCount);
        d.print(F("  lastVal="));       d.println(_lastVal);
        d.print(F("[RECV] All 20:   ")); d.println(all      ? F("PASS") : F("FAIL"));
        d.print(F("[RECV] In order: ")); d.println(_inOrder ? F("PASS") : F("FAIL"));
    }
}

#endif
