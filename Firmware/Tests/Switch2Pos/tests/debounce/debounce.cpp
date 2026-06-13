// Switch2Pos — debounce test
//
// Verifies:
//   poll() is a no-op before forceReport().
//   State change emits EVT only after pin stable for >= 20 ms.
//   Sub-window poll() produces no EVT.
//   A bounce (state change that reverses before window expires) resets the timer.
//   After bounce, confirmed state is still the original — no EVT fires.
//
// Hardware: STM32. PB0→PA0 jumper wire required.
// PB0: output — drives switch state. PA0: input — Switch2Pos reads this.

#include <Arduino.h>
#include <STM32Board.h>
#include <Switch2Pos.h>

static constexpr uint16_t CTRL_ID  = 0xABCD;
static constexpr uint8_t  PIN_CTRL = PB0;
static constexpr uint8_t  PIN_SW   = PA0;

static uint8_t  gEvtCount = 0;
static uint16_t gLastVal  = 0xFFFF;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != canIdEvt(NODE_ID) || len < 8) return;
    const ControlPacketPair* pair = reinterpret_cast<const ControlPacketPair*>(data);
    if (pair->a.controlId == CTRL_ID) {
        gEvtCount++;
        gLastVal = pair->a.value;
    } else if (pair->b.controlId == CTRL_ID) {
        gEvtCount++;
        gLastVal = pair->b.value;
    }
}

static void flushDrain() {
    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    delay(2);
    CANProtocol::drain();
}

OpenSkyhawk::Switch2Pos gSw(CTRL_ID, PinRef(PIN_SW));

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== Switch2Pos debounce ===");
    STM32Board::diagSerial().println("Hardware: PB0->PA0 jumper wire required.");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    pinMode(PIN_CTRL, OUTPUT);
    gSw.configure();

    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptId(canIdEvt(NODE_ID));
    CANProtocol::startLoopback();

    // ── poll() no-op before forceReport() ────────────────────────────────────

    digitalWrite(PIN_CTRL, HIGH);  // inactive
    delayMicroseconds(100);
    for (int i = 0; i < 5; i++) gSw.poll();
    flushDrain();
    check("poll() no-op before forceReport()", gEvtCount == 0);

    // ── Init: pin HIGH (inactive, value=0) ───────────────────────────────────

    gSw.forceReport();
    flushDrain();
    check("forceReport() with HIGH: 1 EVT (value 0)", gEvtCount == 1 && gLastVal == 0);
    uint8_t baseCount = gEvtCount;

    // ── Phase A: clean transition, debounce timing ───────────────────────────
    // Drive pin LOW (active). Confirm EVT does NOT fire before 20 ms,
    // but DOES fire after >= 20 ms.

    digitalWrite(PIN_CTRL, LOW);
    delayMicroseconds(100);

    gSw.poll();          // starts debounce timer
    flushDrain();
    check("immediate poll() after state change: no EVT", gEvtCount == baseCount);

    delay(15);           // 15 ms — still within debounce window
    gSw.poll();
    flushDrain();
    check("poll() at 15 ms: no EVT (debounce not expired)", gEvtCount == baseCount);

    delay(10);           // 25 ms total since state change — window expired
    gSw.poll();
    flushDrain();
    check("poll() at 25 ms: EVT fires (value 1)", gEvtCount == baseCount + 1 && gLastVal == 1);
    baseCount = gEvtCount;

    // ── Phase B: bounce resets timer — no EVT ────────────────────────────────
    // Pin is currently LOW (_lastConfirmed = true/active).
    // Drive HIGH (inactive) — start debounce for inactive state.
    // Before window expires, bounce back LOW (active) — which is the confirmed state.
    // After settling back at LOW, confirm no EVT fires.

    digitalWrite(PIN_CTRL, HIGH);  // start transition to inactive
    delayMicroseconds(100);
    gSw.poll();                    // _pending=false, timer starts T0

    delay(10);                     // 10 ms — debounce not yet expired

    // Bounce back to active (confirmed state)
    digitalWrite(PIN_CTRL, LOW);
    delayMicroseconds(100);
    gSw.poll();                    // raw=true ≠ _pending=false → reset timer, _pending=true
                                   // _pending=true == _lastConfirmed=true → back at confirmed state
    delay(25);                     // wait past debounce window
    gSw.poll();                    // _pending == _lastConfirmed — nothing to confirm
    flushDrain();
    check("bounce back to confirmed: no EVT", gEvtCount == baseCount);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
