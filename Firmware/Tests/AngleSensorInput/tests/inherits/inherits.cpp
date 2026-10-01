// AngleSensorInput — inherited behaviour test
//
// The subclass adds the angle meaning and nothing else: filtering, hysteresis and emission still
// belong to AnalogInput and must work identically when reached through AngleSensorInput. This is
// the check that the readRaw() hook did not move behaviour into the subclass.
//
//   forceReport() emits one baseline EVT and seeds the EWMA.
//   A movement smaller than the hysteresis (128 output counts) stays silent however long it is
//   polled — the sensor sitting still must not chatter on the bus.
//   A larger movement emits, and the smoothed value approaches the new reading.
//
// centerDeg 180 / travelDeg 150 → ~27300 raw counts span the full 0..65535 output, so one raw
// count is about 2.4 output counts: +20 raw stays under the hysteresis, +400 raw clears it.
//
// Rig: this STM32 alone, CAN in silent loopback (no bus, no PanelBridge, no sensor). Do NOT run it
// on a board wired to a live bus: a loopback node never ACKs, which drives the bridge
// error-passive. PASS/FAIL comes from the input's own test seams, not from CAN.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AngleSensorInput/AngleSensorInput.h>

static constexpr uint16_t CTRL_ID = 0x567E;

static uint16_t deg(float d) { return (uint16_t)(d * 65536.0f / 360.0f); }

OpenSkyhawk::AngleSensorInput gAngle(CTRL_ID, PinRef(PA1), /*centerDeg=*/180.0f, /*travelDeg=*/150.0f);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AngleSensorInput inherits ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gAngle.configure();
    CANProtocol::startLoopback();

    const uint16_t centre = deg(180);
    gAngle.debugSetRaw(centre); gAngle.forceReport();
    check("forceReport: one baseline EVT ", gAngle.emitCount() == 1);
    const uint16_t baseline = gAngle.value();

    // Below the hysteresis: polled 40 times, still silent.
    gAngle.debugSetRaw((uint16_t)(centre + 20));
    for (uint8_t i = 0; i < 40; i++) gAngle.debugStep();
    check("small move: no EVT            ", gAngle.emitCount() == 1);
    check("small move: last sent unchanged", gAngle.value() == baseline);

    // Clear of the hysteresis: emits, and the smoothed value tracks the new reading.
    gAngle.debugSetRaw((uint16_t)(centre + 400));
    for (uint8_t i = 0; i < 40; i++) gAngle.debugStep();
    check("large move: emitted           ", gAngle.emitCount() > 1);
    check("large move: value rose        ", gAngle.value() > baseline);
    check("large move: smoothed near new ", gAngle.smoothed() > baseline + 500);

    // Back to the centre: emits again, no stuck state.
    const uint16_t beforeReturn = gAngle.emitCount();
    gAngle.debugSetRaw(centre);
    for (uint8_t i = 0; i < 40; i++) gAngle.debugStep();
    check("return to centre: emitted     ", gAngle.emitCount() > beforeReturn);
    check("return to centre: near baseline",
          gAngle.value() > baseline - 600 && gAngle.value() < baseline + 600);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
