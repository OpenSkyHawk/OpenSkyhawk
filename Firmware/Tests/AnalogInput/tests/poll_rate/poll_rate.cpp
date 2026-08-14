// AnalogInput — poll_rate test
//
// pollMs is PER INSTANCE. Two AnalogInputs on one node, constructed with different pollMs, must
// each hold their own read rate concurrently — neither starving the other, neither clamped back to
// the class default. This is the only env that observes the throttle INTERVAL: debugStep() bypasses
// it by design, so wiring pollMs into the constructor while leaving AnalogInput.cpp comparing a
// class constant still passes all seven other envs (test_force_report's "poll() right after
// forceReport emits nothing" holds at pollMs 8, at pollMs 0, and with the parameter ignored). The
// fast:slow ratio is what kills that mutation — it collapses to 1:1 when both read at 8 ms.
//
// Counting READS, not EVTs. readCount() is a separate seam from emitCount() because they are not
// the same thing once hysteresis is involved: both instances hold a constant injected raw, so the
// EWMA is settled, shouldEmit() is false, and the window emits nothing while reading at full rate.
// That also means the measurement window puts ZERO frames on the CAN bus. Counting emissions would
// need a moving value (~750 EVT/s over a shared bus) and would measure hysteresis and the EWMA ramp
// rather than the throttle.
//
// NOTHING PRINTS INSIDE THE WINDOW — the loop is two poll() calls and nothing else, and debug is
// off so emit() cannot write to USART1 either. This is deliberate, not incidental: setDebug(true)
// caps the chain near 285 Hz on blocking ~20-char writes at 115200, which is BELOW the 470 floor
// asserted for the fast channel. A stray print inside the window would fail this test for a reason
// that has nothing to do with the throttle. Counts are printed before and after, never during.
//
// OMITTING PanelGroup::loop() IS DELIBERATE — do not "fix" this by adding it. Calling the two
// inputs directly is what isolates the throttle; PanelGroup::loop() would fold CAN drain, the
// heartbeat and expander refresh into a timing measurement as jitter. PanelGroup calls poll() on
// every input every iteration with no analog gate of its own (PanelGroup.cpp), so routing through
// it would add noise and prove nothing extra here.
//
// LIMIT, read honestly: debugSetRaw replaces only _pin.readAnalog(). The throttle, EWMA and emit
// gate are production paths, but the cost of a real ADC conversion is not measured. An ADS1115
// PinRef blocks ~8 ms per conversion and cannot sustain pollMs < 8 whatever this reports — see
// TechSpec/PanelGroup/PinRef.md.
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No jumpers / pot needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogInput/AnalogInput.h>

static constexpr uint16_t CTRL_FAST = 0x567B;
static constexpr uint16_t CTRL_SLOW = 0x567C;

static constexpr uint16_t RAW_HELD  = 30000;   // mid-scale: settled EWMA, clear of both rail clauses
static constexpr uint32_t WINDOW_MS = 1000;    // nominal 500 fast reads / 125 slow reads

// Same node, same measurement window, different throttles.
OpenSkyhawk::AnalogInput gFast(CTRL_FAST, PinRef(PA0), /*reverse=*/false, 0, 65535,
                               OpenSkyhawk::AnalogInput::DEFAULT_HYSTERESIS,
                               OpenSkyhawk::AnalogInput::DEFAULT_EWMA_SHIFT, /*pollMs=*/2);
OpenSkyhawk::AnalogInput gSlow(CTRL_SLOW, PinRef(PA1));   // default pollMs = DEFAULT_POLL_MS (8)

void setup() {
    STM32Board::setDebug(false);   // MANDATORY — see the header note on prints inside the window
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();   // works with debug off; begin() starts it unconditionally
    d.println("=== AnalogInput poll_rate ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        d.print(label);
        d.println(ok ? ": PASS" : ": FAIL");
    };

    gFast.configure();
    gSlow.configure();
    CANProtocol::start();

    // Hold both inputs at a constant reading, then take the baseline. forceReport() sets
    // _initialized (poll() is a no-op before it) and seeds the EWMA to RAW_HELD, so the window
    // starts settled and emits nothing.
    gFast.debugSetRaw(RAW_HELD); gFast.forceReport();
    gSlow.debugSetRaw(RAW_HELD); gSlow.forceReport();

    const uint32_t f0 = gFast.readCount();
    const uint32_t s0 = gSlow.readCount();
    const uint16_t e0 = (uint16_t)(gFast.emitCount() + gSlow.emitCount());

    const uint32_t t0 = millis();
    while (millis() - t0 < WINDOW_MS) { gFast.poll(); gSlow.poll(); }
    const uint32_t elapsed = millis() - t0;

    const uint32_t fast = gFast.readCount() - f0;
    const uint32_t slow = gSlow.readCount() - s0;

    d.print(F("window "));                 d.print(elapsed);
    d.print(F(" ms  fast(pollMs=2)="));    d.print(fast);
    d.print(F("  slow(pollMs=8)="));       d.println(slow);

    // The throttle re-arms from `now`, not from _lastReadMs + pollMs, so the achieved rate can only
    // sit at or below nominal; millis() granularity plus loop overhead costs at most a tick per
    // read. A FAIL that halves a rate means the loop period exceeded pollMs — a true finding, not
    // a flaky test. The failure modes this exists to catch miss by 2x or 4x, far outside the bands.
    check("pollMs=2 -> ~500 reads/s", fast >= 470 && fast <= 501);
    check("pollMs=8 -> ~125 reads/s", slow >= 118 && slow <= 126);

    // The mutation-killer: if poll() still compares a class constant, both read at 8 ms and this
    // collapses to 1:1. Per instance, not per node — stated so a failure names the real property.
    check("fast is ~4x slow (independent throttles)",
          fast * 10 >= slow * 32 && fast * 10 <= slow * 48);

    // Confirms the window measured the throttle and not CAN backpressure — and that it cost no bus
    // traffic at all.
    check("held value emits nothing at either rate",
          (uint16_t)(gFast.emitCount() + gSlow.emitCount()) == e0);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    d.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
