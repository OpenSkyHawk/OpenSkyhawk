// Switch2Pos — reverse polarity test
//
// Verifies:
//   reverse=true: pin HIGH (driven HIGH) → EVT value 1 (active).
//   reverse=true: pin LOW  (driven LOW)  → EVT value 0 (inactive).
//   reverse=false (default): pin LOW → value 1, pin HIGH → value 0 (confirmed).
//   forceReport() and poll() both respect the reverse flag.
//
// Hardware: STM32. PB0→PA0 jumper wire required.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/Switch2Pos/Switch2Pos.h>

static constexpr uint16_t CTRL_FWD = 0xAA01;  // reverse=false
static constexpr uint16_t CTRL_REV = 0xAA02;  // reverse=true
static constexpr uint8_t  PIN_CTRL = PB0;
static constexpr uint8_t  PIN_SW   = PA0;

static uint16_t gFwdVal = 0xFFFF;
static uint16_t gRevVal = 0xFFFF;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != canIdEvt(NODE_ID) || len < 8) return;
    const ControlPacketPair* pair = reinterpret_cast<const ControlPacketPair*>(data);
    auto check_slot = [&](const ControlPacket& pkt) {
        if (pkt.controlId == CTRL_FWD) gFwdVal = pkt.value;
        if (pkt.controlId == CTRL_REV) gRevVal = pkt.value;
    };
    check_slot(pair->a);
    check_slot(pair->b);
}

static void flushDrain() {
    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    delay(2);
    CANProtocol::drain();
}

// Two switches on the same physical pin — one fwd, one reversed.
OpenSkyhawk::Switch2Pos gSwFwd(CTRL_FWD, PinRef(PIN_SW));               // reverse=false
OpenSkyhawk::Switch2Pos gSwRev(CTRL_REV, PinRef(PIN_SW), /*reverse=*/true);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== Switch2Pos reverse ===");
    STM32Board::diagSerial().println("Hardware: PB0->PA0 jumper wire required.");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    pinMode(PIN_CTRL, OUTPUT);
    gSwFwd.configure();
    gSwRev.configure();

    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptId(canIdEvt(NODE_ID));
    CANProtocol::startLoopback();

    // ── Pin HIGH ─────────────────────────────────────────────────────────────
    // reverse=false: HIGH → inactive → value 0
    // reverse=true:  HIGH → active   → value 1

    digitalWrite(PIN_CTRL, HIGH);
    delayMicroseconds(100);
    gSwFwd.forceReport();
    gSwRev.forceReport();
    flushDrain();

    check("reverse=false, HIGH: value 0", gFwdVal == 0);
    check("reverse=true,  HIGH: value 1", gRevVal == 1);

    // ── Pin LOW ──────────────────────────────────────────────────────────────
    // reverse=false: LOW → active   → value 1
    // reverse=true:  LOW → inactive → value 0

    digitalWrite(PIN_CTRL, LOW);
    delayMicroseconds(100);
    gSwFwd.forceReport();
    gSwRev.forceReport();
    flushDrain();

    check("reverse=false, LOW: value 1", gFwdVal == 1);
    check("reverse=true,  LOW: value 0", gRevVal == 0);

    // ── poll() respects reverse flag ──────────────────────────────────────────
    // Transition pin HIGH. After debounce, reverse=false → value 0, reverse=true → value 1.

    digitalWrite(PIN_CTRL, HIGH);
    delayMicroseconds(100);
    gSwFwd.poll();
    gSwRev.poll();
    delay(25);
    gSwFwd.poll();
    gSwRev.poll();
    flushDrain();

    check("poll() reverse=false, HIGH: value 0", gFwdVal == 0);
    check("poll() reverse=true,  HIGH: value 1", gRevVal == 1);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
