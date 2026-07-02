// ShiftBus — bench gate 6: EC11 encoder fidelity through a real 74HC165
//
// THE decision gate (#197): spin the EC11s fast — zero missed counts?
// Two encoders on '165 chip 0: enc1 A=D0 B=D1 (REL), enc2 A=D2 B=D3 (DIR).
// 10 k pull-ups on every input; encoder commons to GND.
//
// Build twice and compare (same sketch, two envs):
//   bench_gate6_encoder      — loop-poll decode (today's mode)
//   bench_gate6_encoder_isr  — timer-ISR decode @SHIFTBUS_ISR_HZ (the #197 fix)
//
// STALL_MS (default 25) fakes the ASN-41 OLED sendBuffer flush: every STALL_PERIOD_MS the
// loop blocks for STALL_MS, exactly when fidelity breaks in loop-poll mode. Set
// -DSTALL_MS=0 for a clean-loop control run.
//
// Procedure per env: note "detent total" on the diag serial, spin exactly N full
// revolutions fast (20-detent EC11 → expect 20·N), compare. Repeat while the stall runs.
// PASS: ISR env total == physical detents (both directions); loop-poll env expected to
// drop/blip during stalls — that delta is the measurement.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>

#ifndef STALL_MS
#define STALL_MS 25
#endif
static constexpr uint32_t STALL_PERIOD_MS = 100;   // ~10 flushes/s, ASN-41-ish while cranking

static constexpr uint16_t CTRL_REL = 0x3B01;   // bench ids
static constexpr uint16_t CTRL_DIR = 0x3B02;

OpenSkyhawk::RotaryEncoder gEnc1(CTRL_REL, PinRef(ShiftBus1, 0, 0), PinRef(ShiftBus1, 0, 1),
                                 OpenSkyhawk::EncoderStepsPerDetent::Four,
                                 OpenSkyhawk::EncoderMode::Rel);
OpenSkyhawk::RotaryEncoder gEnc2(CTRL_DIR, PinRef(ShiftBus1, 0, 2), PinRef(ShiftBus1, 0, 3),
                                 OpenSkyhawk::EncoderStepsPerDetent::Four,
                                 OpenSkyhawk::EncoderMode::Dir);

void setup() {
    STM32Board::setDebug(true);
    PanelGroup::setup();
    auto& d = STM32Board::diagSerial();
    d.println("=== ShiftBus bench gate 6: encoder fidelity ===");
#ifdef SHIFTBUS_ISR_HZ
    d.print(F("mode: ISR @")); d.print(SHIFTBUS_ISR_HZ); d.println(F(" Hz"));
#else
    d.println(F("mode: loop-poll"));
#endif
    d.print(F("stall: ")); d.print(STALL_MS); d.print(F(" ms every "));
    d.print(STALL_PERIOD_MS); d.println(F(" ms"));
}

void loop() {
    PanelGroup::loop();

#if STALL_MS > 0
    // Fake OLED flush: block the loop, exactly like a u8g2 sendBuffer at 100 kHz I2C.
    static uint32_t lastStall = 0;
    if (millis() - lastStall >= STALL_PERIOD_MS) {
        lastStall = millis();
        delay(STALL_MS);
    }
#endif

    // Running totals every 2 s — spin exactly N revolutions and compare.
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint >= 2000) {
        lastPrint = millis();
        auto& d = STM32Board::diagSerial();
        d.print(F("[gate6] enc1(REL) detents=")); d.print((int32_t)gEnc1.netDetents());
        d.print(F("  enc2(DIR) detents="));       d.println((int32_t)gEnc2.netDetents());
    }
}
