// ShiftBus — isr_pending test (chip-less bypass + REAL timer ISR at SHIFTBUS_ISR_HZ=1000)
//
// The gate-6 mechanism in software: the hardware timer samples the (injected) bus frame and
// decodes in ISR context while the "loop" deliberately never polls — then a single poll()
// drains everything. Proves decode fidelity is decoupled from poll cadence: detents fed
// during a poll-free stall all arrive, REL coalesced into one frame, DIR one frame per detent.
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No shift registers needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>

static constexpr uint16_t CTRL_REL = 0x1111;
static constexpr uint16_t CTRL_DIR = 0x2222;

OpenSkyhawk::RotaryEncoder gRel(CTRL_REL, PinRef(ShiftBus1, 0, 1), PinRef(ShiftBus1, 0, 0),
                                OpenSkyhawk::EncoderStepsPerDetent::Four,
                                OpenSkyhawk::EncoderMode::Rel);
OpenSkyhawk::RotaryEncoder gDir(CTRL_DIR, PinRef(ShiftBus1, 0, 3), PinRef(ShiftBus1, 0, 2),
                                OpenSkyhawk::EncoderStepsPerDetent::Four,
                                OpenSkyhawk::EncoderMode::Dir);

static void injectStates(uint8_t relState, uint8_t dirState) {
    // No transfer(), no poll() — the timer ISR picks the new frame up on its next tick.
    ShiftBus1.testInjectIn(0, (uint8_t)((relState & 0x3) | ((dirState & 0x3) << 2)));
    delay(5);   // ≥5 ISR ticks at 1 kHz — repeated samples of a held state decode once
}

// The PanelGroup step-3b wiring, reproduced locally (this test drives the bus directly
// instead of running the full PanelGroup lifecycle): bus ISR → every input's sampleTick().
static void tickAllInputs(void*) {
    for (auto* p = OpenSkyhawk::InputBase::head(); p; p = p->next())
        p->sampleTick();
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println("=== ShiftBus isr_pending (timer ISR @1 kHz, bypass) ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        d.print(label);
        d.println(ok ? ": PASS" : ": FAIL");
    };

    ShiftBus1.testSetBypass(true);
    gRel.configure();
    gDir.configure();
    ShiftBus1.begin();
    CANProtocol::start();

    ShiftBus1.testInjectIn(0, 0x00);
    ShiftBus1.transfer();
    gRel.forceReport();
    gDir.forceReport();

    ShiftBus1.addIsrConsumer(tickAllInputs, nullptr);
    ShiftBus1.beginIsrSampling(TIM2, SHIFTBUS_ISR_HZ);
    check("ISR active", ShiftBus1.isrActive());

    uint16_t t0 = ShiftBus1.testTransferCount();
    delay(50);
    check("timer transfers (~50 ticks)", ShiftBus1.testTransferCount() >= t0 + 40);

    // Feed 3 CW detents (12 Gray states) with ZERO polls — the simulated stalled loop.
    const uint8_t cw[] = {1, 3, 2, 0};
    for (uint8_t rep = 0; rep < 3; rep++)
        for (uint8_t s : cw) injectStates(s, s);

    check("nothing emitted before drain", gRel.emitCount() == 0 && gDir.emitCount() == 0);

    // One poll drains the stall's accumulation.
    gRel.poll();
    gDir.poll();
    check("REL: 3 detents coalesced into ONE frame of 3*step",
          gRel.emitCount() == 1
          && gRel.lastValue() == 3 * OpenSkyhawk::RotaryEncoder::DEFAULT_STEP);
    check("DIR: 3 detents -> three +1 frames", gDir.emitCount() == 3 && gDir.lastValue() == 1);

    // Direction reversal through a stall: 2 CCW detents, drain, expect -2*step / two -1s.
    const uint8_t ccw[] = {2, 3, 1, 0};
    for (uint8_t rep = 0; rep < 2; rep++)
        for (uint8_t s : ccw) injectStates(s, s);
    gRel.poll();
    gDir.poll();
    check("REL reverse: one frame of -2*step",
          gRel.emitCount() == 2
          && gRel.lastValue() == -2 * OpenSkyhawk::RotaryEncoder::DEFAULT_STEP);
    check("DIR reverse: two -1 frames", gDir.emitCount() == 5 && gDir.lastValue() == -1);

    // Steady state: no motion, poll emits nothing.
    delay(20);
    gRel.poll(); gDir.poll();
    check("idle emits nothing", gRel.emitCount() == 2 && gDir.emitCount() == 5);

    CANProtocol::flushBatched(canIdEvtRel(NODE_ID));
    CANProtocol::flushBatched(canIdEvtDir(NODE_ID));
    d.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
