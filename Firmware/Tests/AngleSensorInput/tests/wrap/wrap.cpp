// AngleSensorInput — 0°/360° wrap test
//
// The reason the class re-centres instead of using the raw angle: a knob whose travel straddles the
// sensor's zero must behave like any other. centerDeg 10 / travelDeg 60 spans 340°..40°, across the
// seam.
//
//   350° (10° below centre) and 30° (20° above centre) must sit either side of mid-scale, in that
//   order and with no discontinuity — 350 > 30 as raw counts, but 350° is the *lower* position.
//   340° and 40° are the travel ends → 0 and 65535.
//   190°, the far side of the circle, is outside the travel and must clamp to a rail rather than
//   reading as some mid-travel position.
//
// Rig: this STM32 alone, CAN in silent loopback (no bus, no PanelBridge, no sensor). Do NOT run it
// on a board wired to a live bus: a loopback node never ACKs, which drives the bridge
// error-passive. PASS/FAIL comes from the input's own test seams, not from CAN.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AngleSensorInput/AngleSensorInput.h>

static constexpr uint16_t CTRL_ID = 0x567D;

static uint16_t deg(float d) { return (uint16_t)(d * 65536.0f / 360.0f); }

OpenSkyhawk::AngleSensorInput gAngle(CTRL_ID, PinRef(PA1), /*centerDeg=*/10.0f, /*travelDeg=*/60.0f);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AngleSensorInput wrap ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gAngle.configure();
    CANProtocol::startLoopback();

    gAngle.debugSetRaw(deg(10)); gAngle.forceReport();
    const uint16_t atCentre = gAngle.value();
    check("centre 10 deg -> mid-scale    ", atCentre > 32000 && atCentre < 33500);

    gAngle.debugSetRaw(deg(350)); gAngle.forceReport();
    const uint16_t below = gAngle.value();
    check("350 deg -> below centre       ", below < atCentre && below > 0);

    gAngle.debugSetRaw(deg(30)); gAngle.forceReport();
    const uint16_t above = gAngle.value();
    check("30 deg -> above centre        ", above > atCentre && above < 65535);

    // 350° is 20° below centre, 30° is 20° above: symmetric around mid-scale.
    const uint16_t downSpan = (uint16_t)(atCentre - below);
    const uint16_t upSpan   = (uint16_t)(above - atCentre);
    const uint16_t diff     = downSpan > upSpan ? downSpan - upSpan : upSpan - downSpan;
    check("350/30 symmetric about centre ", diff < 1500);

    gAngle.debugSetRaw(deg(340)); gAngle.forceReport();
    check("340 deg (travel start) -> 0   ", gAngle.value() == 0);

    gAngle.debugSetRaw(deg(40)); gAngle.forceReport();
    check("40 deg (travel end) -> 65535  ", gAngle.value() == 65535);

    // The far side of the circle is out of travel: a rail, not a mid-travel reading.
    gAngle.debugSetRaw(deg(190)); gAngle.forceReport();
    const uint16_t farSide = gAngle.value();
    check("190 deg -> clamped to a rail  ", farSide == 0 || farSide == 65535);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
