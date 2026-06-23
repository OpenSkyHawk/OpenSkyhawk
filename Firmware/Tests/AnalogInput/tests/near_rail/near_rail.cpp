// AnalogInput — near_rail test (strengthened, audit #11)
//
// shouldEmit() force-sends a sub-hysteresis move when it heads INTO a rail band (0 / 65535), so full
// travel always reaches the endpoint. The first two checks ISOLATE that clause: seed _lastSent just
// off a rail, then move toward it by LESS than the 128-count hysteresis — only the near-rail term can
// emit (the normal threshold suppresses a sub-hysteresis move), proven by emitCount() incrementing.
// A stateless drive-the-rail check (the original test) would pass even if the rail clause regressed,
// because the full swing also clears the hysteresis. The last two checks confirm a full sweep lands.
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No jumpers / pot needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogInput/AnalogInput.h>

static constexpr uint16_t CTRL_ID = 0x567A;

OpenSkyhawk::AnalogInput gAna(CTRL_ID, PinRef(PA1));   // defaults: hysteresis 128, EWMA shift 3

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogInput near_rail ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gAna.configure();
    CANProtocol::start();

    // --- ISOLATION: a sub-hysteresis move INTO the top rail still emits (near-rail clause only) ---
    gAna.debugSetRaw(65500); gAna.forceReport();        // settle just below the top rail
    uint16_t c = gAna.emitCount();
    gAna.debugSetRaw(65535); gAna.debugStep();          // smoothed rises ~4 counts (< 128 hysteresis)
    check("top: sub-hysteresis move into rail emits (clause isolated)",
          gAna.emitCount() == (uint16_t)(c + 1) && gAna.value() > 65500);

    // --- ISOLATION: a sub-hysteresis move INTO the bottom rail still emits ---
    gAna.debugSetRaw(100); gAna.forceReport();          // settle just above the bottom rail
    c = gAna.emitCount();
    gAna.debugSetRaw(0); gAna.debugStep();              // smoothed falls ~13 counts (< 128 hysteresis)
    check("bottom: sub-hysteresis move into rail emits (clause isolated)",
          gAna.emitCount() == (uint16_t)(c + 1) && gAna.value() < 100);

    // --- a full sweep actually lands on each rail ---
    gAna.debugSetRaw(30000); gAna.forceReport();        // mid baseline
    gAna.debugSetRaw(65535);
    for (int i = 0; i < 80; i++) gAna.debugStep();      // climb to the top rail
    check("full climb reaches top rail", gAna.value() > 65535 - 200 && gAna.smoothed() > 65535 - 100);

    gAna.debugSetRaw(0);
    for (int i = 0; i < 80; i++) gAna.debugStep();      // fall to the bottom rail
    check("full fall reaches bottom rail", gAna.value() < 200 && gAna.smoothed() < 100);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
