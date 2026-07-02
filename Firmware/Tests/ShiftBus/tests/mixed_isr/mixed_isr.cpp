// ShiftBus — mixed_isr test (chip-less bypass + REAL timer ISR at SHIFTBUS_ISR_HZ)
//
// The Codex/PR-review regression case: a mixed node (SR + GPIO encoders) with ISR
// sampling active. The SR-pinned encoder must be sampler-owned (decode at the tick rate,
// poll only drains); the GPIO-pinned encoder must DECLINE sampler ownership and keep its
// loop-rate decode exactly as before — SHIFTBUS_ISR_HZ has no business rewiring it.
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No shift registers
// (bypass) and no physical encoders (GPIO encoder driven through the debug seams).

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>

static constexpr uint16_t CTRL_SR   = 0x1111;
static constexpr uint16_t CTRL_GPIO = 0x2222;

OpenSkyhawk::RotaryEncoder gSr(CTRL_SR, PinRef(ShiftBus1, 0, 1), PinRef(ShiftBus1, 0, 0),
                               OpenSkyhawk::EncoderStepsPerDetent::Four,
                               OpenSkyhawk::EncoderMode::Rel);
OpenSkyhawk::RotaryEncoder gGpio(CTRL_GPIO, PinRef(PA0), PinRef(PA1),
                                 OpenSkyhawk::EncoderStepsPerDetent::Four,
                                 OpenSkyhawk::EncoderMode::Rel);

// PanelGroup step-3b wiring, reproduced locally: bus ISR → every input's sampleTick().
static void tickAllInputs(void*) {
    for (auto* p = OpenSkyhawk::InputBase::head(); p; p = p->next())
        p->sampleTick();
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println("=== ShiftBus mixed_isr (SR sampler-owned, GPIO stays loop-decoded) ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        d.print(label);
        d.println(ok ? ": PASS" : ": FAIL");
    };

    ShiftBus1.testSetBypass(true);
    gSr.configure();
    gGpio.configure();
    ShiftBus1.begin();
    CANProtocol::start();

    ShiftBus1.testInjectIn(0, 0x00);
    ShiftBus1.transfer();
    gSr.forceReport();
    gGpio.forceReport();

    ShiftBus1.addIsrConsumer(tickAllInputs, nullptr);
    ShiftBus1.beginIsrSampling(TIM2, SHIFTBUS_ISR_HZ);
    check("ISR active", ShiftBus1.isrActive());
    delay(20);   // ≥20 ticks — every input's sampleTick() has fired repeatedly by now

    // GPIO encoder: sampler must have DECLINED ownership — loop-rate decode via the
    // debug seams still emits, exactly as on a node without any ShiftBus.
    gGpio.debugSeed(0);
    gGpio.debugStep(1); gGpio.debugStep(3); gGpio.debugStep(2); gGpio.debugStep(0);  // CW detent
    check("GPIO encoder still loop-decodes under ISR",
          gGpio.emitCount() == 1
          && gGpio.lastValue() == OpenSkyhawk::RotaryEncoder::DEFAULT_STEP);

    // SR encoder: sampler-owned — Gray states fed through the injected frame decode at
    // the tick rate with zero polls, one coalesced emit on drain.
    const uint8_t cw[] = {1, 3, 2, 0};
    for (uint8_t rep = 0; rep < 2; rep++)
        for (uint8_t s : cw) { ShiftBus1.testInjectIn(0, s & 0x3); delay(5); }
    check("SR encoder nothing before drain", gSr.emitCount() == 0);
    gSr.poll();
    check("SR encoder sampler-owned, coalesced 2 detents",
          gSr.emitCount() == 1
          && gSr.lastValue() == 2 * OpenSkyhawk::RotaryEncoder::DEFAULT_STEP);

    // And the GPIO encoder keeps working after SR activity — no cross-ownership bleed.
    gGpio.debugStep(2); gGpio.debugStep(3); gGpio.debugStep(1); gGpio.debugStep(0);  // CCW detent
    check("GPIO encoder unaffected after SR traffic",
          gGpio.emitCount() == 2
          && gGpio.lastValue() == -OpenSkyhawk::RotaryEncoder::DEFAULT_STEP);

    CANProtocol::flushBatched(canIdEvtRel(NODE_ID));
    d.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
