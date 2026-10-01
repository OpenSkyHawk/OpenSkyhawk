// Switch2Pos — debounce test
//
// Verifies:
//   poll() is a no-op before forceReport().
//   State change emits EVT only after pin stable for >= 20 ms.
//   Sub-window poll() produces no EVT.
//   Bounce back to confirmed state: debounce timer resets; no EVT fires.
//   Bounce through confirmed state then settle at new state: timer resets
//     on EACH raw change; EVT fires only after full 20 ms from final settle.
//
// Hardware: STM32. PB0→PA0 jumper wire required.
// PB0: output — drives switch state. PA0: input — Switch2Pos reads this.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/Switch2Pos/Switch2Pos.h>

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
    // Drive pin LOW (active). Confirm EVT does NOT fire before DEBOUNCE_MS, but does after.
    //
    // Elapsed time is MEASURED here, never assumed. A PASS line is ~50 characters, which at
    // 115200 baud is ~4 ms of transmit the test does not control, and flushDrain() adds ~2 ms
    // more. An earlier version budgeted only the flushDrain and polled at what it called "17 ms"
    // — measured on hardware that poll landed at 22-26 ms, past the window, so the EVT fired
    // correctly and the assertion failed. Reading millis() instead makes the test immune to
    // however long the preceding output happens to be.

    digitalWrite(PIN_CTRL, LOW);
    delayMicroseconds(100);

    const uint32_t t0 = millis();  // the poll below starts the debounce timer here
    gSw.poll();

    // Inside the window: poll repeatedly, every one provably before DEBOUNCE_MS. No printing in
    // the loop — that is what used to push the last poll past the edge.
    while ((uint32_t)(millis() - t0) < OpenSkyhawk::Switch2Pos::DEBOUNCE_MS - 4) gSw.poll();
    flushDrain();
    check("inside debounce window: no EVT", gEvtCount == baseCount);

    // Past the window: the next poll must confirm.
    while ((uint32_t)(millis() - t0) < OpenSkyhawk::Switch2Pos::DEBOUNCE_MS + 4) { /* wait out the edge */ }
    gSw.poll();
    flushDrain();
    check("past debounce window: EVT fires (value 1)", gEvtCount == baseCount + 1 && gLastVal == 1);
    baseCount = gEvtCount;

    // ── Phase B: bounce back to confirmed state — no EVT ─────────────────────
    // Pin currently LOW (_lastConfirmed=true/active).
    // Drive HIGH (inactive) → _pending=false, timer resets.
    // Before window expires, bounce back LOW (active = confirmed state).
    // _pending flips back to true (== _lastConfirmed) → no confirmation possible.
    // Even after waiting, no EVT fires.

    digitalWrite(PIN_CTRL, HIGH);
    delayMicroseconds(100);
    gSw.poll();                    // _pending=false, timer starts

    delay(10);                     // 10 ms — debounce not yet expired

    digitalWrite(PIN_CTRL, LOW);   // bounce back to confirmed state
    delayMicroseconds(100);
    gSw.poll();                    // raw=true ≠ _pending=false → reset timer, _pending=true
                                   // _pending=true == _lastConfirmed=true — back at confirmed
    delay(25);
    gSw.poll();                    // _pending == _lastConfirmed → nothing to confirm
    flushDrain();
    check("Phase B: bounce-to-confirmed, no EVT", gEvtCount == baseCount);

    // ── Phase C: bounce through confirmed, settle at new state ───────────────
    // Same start: _lastConfirmed=true (LOW/active).
    // Bounce sequence: confirmed→inactive→confirmed→inactive(settle).
    // Each raw change resets the timer. EVT fires only after the LAST direction
    // change has been stable for >= 20 ms.

    digitalWrite(PIN_CTRL, HIGH);  // step 1: go inactive
    delayMicroseconds(100);
    gSw.poll();                    // _pending=false, timer T0

    delay(10);                     // 10 ms

    digitalWrite(PIN_CTRL, LOW);   // step 2: bounce back to confirmed (active)
    delayMicroseconds(100);
    gSw.poll();                    // _pending=true (== _lastConfirmed), timer T1

    delay(10);                     // 10 ms

    digitalWrite(PIN_CTRL, HIGH);  // step 3: go inactive again (settle here)
    delayMicroseconds(100);
    gSw.poll();                    // _pending=false (!= _lastConfirmed), timer T2

    // At this point, 20 ms has NOT yet elapsed since T2.
    flushDrain();
    check("Phase C: before settle window: no EVT", gEvtCount == baseCount);

    delay(25);                     // >= 20 ms since T2
    gSw.poll();                    // confirms inactive → EVT value=0
    flushDrain();
    check("Phase C: after settle window: EVT fires (value 0)", gEvtCount == baseCount + 1 && gLastVal == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
