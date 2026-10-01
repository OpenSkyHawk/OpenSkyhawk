// AngleSensorInput — degree mapping test
//
// centerDeg 180 / travelDeg 150 → the knob spans 105°..255°, which must come out as 0..65535 with
// the centre at mid-scale. A full turn is 65536 counts, so one degree is 65536/360 ≈ 182 counts:
//   180° = 32768 raw → ~32767 out (centre)
//   105° = 19114 raw → 0      (lower end of travel)
//   255° = 46421 raw → 65535  (upper end)
//   outside 105°..255° → clamped to the rail it passed, not wrapped to the other end
//
// debugSetRaw injects the reading (no sensor needed) and forceReport() seeds the EWMA, so value()
// is the scaled result immediately — the same idiom as the AnalogInput envs.
//
// Rig: this STM32 alone, CAN in silent loopback (no bus, no PanelBridge, no sensor). Do NOT run it
// on a board wired to a live bus: a loopback node never ACKs, which drives the bridge
// error-passive. PASS/FAIL comes from the input's own test seams, not from CAN.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AngleSensorInput/AngleSensorInput.h>

static constexpr uint16_t CTRL_ID = 0x567C;

// Counts per degree, as the class computes them.
static uint16_t deg(float d) { return (uint16_t)(d * 65536.0f / 360.0f); }

OpenSkyhawk::AngleSensorInput gAngle(CTRL_ID, PinRef(PA1), /*centerDeg=*/180.0f, /*travelDeg=*/150.0f);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AngleSensorInput mapping ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gAngle.configure();
    CANProtocol::startLoopback();

    gAngle.debugSetRaw(deg(180)); gAngle.forceReport();
    check("centre 180 deg -> mid-scale   ", gAngle.value() > 32000 && gAngle.value() < 33500);

    gAngle.debugSetRaw(deg(105)); gAngle.forceReport();
    check("105 deg (centre-75) -> 0      ", gAngle.value() == 0);

    gAngle.debugSetRaw(deg(255)); gAngle.forceReport();
    check("255 deg (centre+75) -> 65535  ", gAngle.value() == 65535);

    // Past the travel on either side: clamp, never wrap to the opposite rail.
    gAngle.debugSetRaw(deg(90));  gAngle.forceReport();
    check("90 deg (below travel) -> 0    ", gAngle.value() == 0);

    gAngle.debugSetRaw(deg(270)); gAngle.forceReport();
    check("270 deg (above travel) -> full", gAngle.value() == 65535);

    // A quarter into the travel should land about a quarter up the output.
    gAngle.debugSetRaw(deg(142.5f)); gAngle.forceReport();
    check("142.5 deg -> ~25%             ", gAngle.value() > 15000 && gAngle.value() < 18500);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
