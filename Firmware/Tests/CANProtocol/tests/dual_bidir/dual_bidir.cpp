// CANProtocol — bidirectional simultaneous TX+RX integration test
//
// Requires two boards on a physical CAN bus with 120 Ω termination at each end.
// Both boards can be powered simultaneously.
//
// Both nodes send their own heartbeat ID at 50 ms intervals and count received
// heartbeats from the other node. This exercises:
//   - Full-duplex CAN (TX and RX at the same time on both nodes)
//   - Bus arbitration (simultaneous transmissions resolved by ID priority)
//   - Symmetric bus wiring (either board can be either node)
//
// NODE_ID=0: sends canIdHb(0)=0x100, counts received canIdHb(1)=0x101
// NODE_ID=1: sends canIdHb(1)=0x101, counts received canIdHb(0)=0x100
//
// Prints stats every second for 10 s, then final PASS/FAIL.
// Expected: ~20 tx and ~20 rx per second, drops=0 throughout.
// PASS: rx_count >= 160 (80% of expected 200 over 10 s), drops=0.
//
// Flash:
//   pio run -e test_dual_bidir_node0 -t upload   (Board A, NODE_ID=0)
//   pio run -e test_dual_bidir_node1 -t upload   (Board B, NODE_ID=1)
// Monitor: open DiagSerial (115200) on both boards.
// Both boards can be powered at the same time.

#include <STM32Board.h>
#include <CANProtocol.h>

static uint32_t _txCount  = 0;
static uint32_t _rxCount  = 0;
static uint32_t _lastTx   = 0;
static uint32_t _lastStat = 0;

static const uint32_t TX_ID = canIdHb(NODE_ID);
static const uint32_t RX_ID = canIdHb(NODE_ID ^ 1);

static void onRx(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId == RX_ID) _rxCount++;
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log(NODE_ID == 0
        ? "=== dual_bidir NODE 0 (TX:0x100 RX:0x101) ==="
        : "=== dual_bidir NODE 1 (TX:0x101 RX:0x100) ===");
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onReceive(onRx);
    CANProtocol::filterAcceptAll();
    CANProtocol::start();
    _lastStat = millis();
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    uint32_t now = millis();

    // Send own heartbeat at 50 ms intervals (~20 Hz)
    if (now - _lastTx >= 50) {
        _lastTx = now;
        uint8_t buf[8] = {};
        buf[0] = (uint8_t)(_txCount & 0xFF);
        CANProtocol::send(TX_ID, buf, 8);
        _txCount++;
    }

    // Print stats every second for 10 s
    static uint8_t _statTick = 0;
    if (_statTick < 10 && now - _lastStat >= 1000) {
        _lastStat = now;
        _statTick++;
        uint32_t drops = CANProtocol::txDropCount();
        auto& d = STM32Board::diagSerial();
        d.print(F("t=")); d.print(_statTick);
        d.print(F("s  tx=")); d.print(_txCount);
        d.print(F("  rx=")); d.print(_rxCount);
        d.print(F("  drops=")); d.println(drops);
    }

    // Final report at t=11 s
    static bool _reported = false;
    if (!_reported && now > 11000) {
        _reported = true;
        uint32_t drops = CANProtocol::txDropCount();
        bool pass = (_rxCount >= 160 && drops == 0);
        auto& d = STM32Board::diagSerial();
        d.println(F("--- FINAL ---"));
        d.print(F("tx=")); d.print(_txCount);
        d.print(F("  rx=")); d.print(_rxCount);
        d.print(F("  drops=")); d.println(drops);
        d.print(F("Bidir: "));
        d.println(pass ? F("PASS") : F("FAIL"));
    }
}
