// AnalogInput — hysteresis test
//
// A settled value within `hysteresis` (128) counts of the last sent value emits nothing; a change
// that settles beyond it emits and the value tracks. debugStep() runs one EWMA step bypassing the
// 8 ms throttle. Verified via emitCount()/value().
//
// Route-aware default (#295): with no hysteresis set, a DCSIN_* id gets DCS_HYSTERESIS (1024) and a
// CTRL_* id HID_HYSTERESIS (128); an explicit value always wins. A Δ = 500 settle tells them apart.
//
// Rig: this STM32 alone, CAN in silent loopback (no bus, no PanelBridge, no pot). Do NOT run it
// on a board wired to a live bus: a loopback node never ACKs, which drives the bridge
// error-passive. PASS/FAIL comes from the input's own test seams, not from CAN.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogInput/AnalogInput.h>

static constexpr uint16_t CTRL_ID = 0x567A;

OpenSkyhawk::AnalogInput gAna(CTRL_ID, PinRef(PA1));   // defaults: hysteresis 128, EWMA shift 3

static constexpr uint16_t DCS_ID = 0x8001;   // in the DCS-BIOS range (DCSIN_*)
static constexpr uint16_t HID_ID = 0x0010;   // in the HID range (CTRL_ROLL)
OpenSkyhawk::AnalogInput gDcs(DCS_ID, PinRef(PA2));                          // auto → 1024
OpenSkyhawk::AnalogInput gHid(HID_ID, PinRef(PA3));                          // auto → 128
OpenSkyhawk::AnalogInput gOver(DCS_ID + 1, PinRef(PA4), { .hysteresis = 128 });  // explicit wins

// Baseline 20000, settle at 20500 (Δ = 500): true if it emitted past the baseline.
static bool emitsOn500(OpenSkyhawk::AnalogInput& a) {
    a.configure();
    a.debugSetRaw(20000); a.forceReport();
    a.debugSetRaw(20500);
    for (int i = 0; i < 60; i++) a.debugStep();
    return a.emitCount() > 1;
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogInput hysteresis ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gAna.configure();
    CANProtocol::startLoopback();

    gAna.debugSetRaw(20000); gAna.forceReport();
    check("baseline: 1 EVT at 20000", gAna.emitCount() == 1 && gAna.value() == 20000);

    // Sub-hysteresis change (Δ = 50 < 128): EWMA settles to 20050 but nothing is emitted.
    gAna.debugSetRaw(20050);
    for (int i = 0; i < 40; i++) gAna.debugStep();
    check("sub-hysteresis change: no EVT", gAna.emitCount() == 1 && gAna.value() == 20000);

    // Supra-hysteresis change: settles at 50000, emits along the way, value() tracks.
    gAna.debugSetRaw(50000);
    for (int i = 0; i < 80; i++) gAna.debugStep();
    check("supra-hysteresis change: emitted + tracks", gAna.emitCount() > 1 && gAna.value() > 49000);

    check("auto, DCSIN id: 1024 -> Δ500 silent", !emitsOn500(gDcs));
    check("auto, CTRL id: 128 -> Δ500 emits",    emitsOn500(gHid));
    check("explicit 128 on a DCSIN id: emits",   emitsOn500(gOver));

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
