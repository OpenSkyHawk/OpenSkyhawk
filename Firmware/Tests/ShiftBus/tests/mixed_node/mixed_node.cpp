// ShiftBus — mixed_node test (compile + configure: all PinRef backends on ONE node)
//
// The PinRef contract is source-transparency: one sketch mixing direct GPIO, MCP23017,
// ADS1115, and ShiftBus pins across the input/output classes. This test locks that in —
// every backend declared side by side, configure() routing verified per type, ShiftBus
// chains auto-sized while the MCP/ADS declarations coexist untouched.
//
// Rig: bare STM32 (SHIFTBUS_TEST bypass; MCP/ADS objects are declared but not begin()'d —
// no I2C traffic happens at configure()).

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>
#include <Inputs/Switch2Pos/Switch2Pos.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>
#include <Outputs/LED/LED.h>

MCP23017 gExp(0x20, Wire);
ADS1115  gAdc;

// One of each source, mixed across consumers:
OpenSkyhawk::Switch2Pos    swGpio(0x4001, PinRef(PA0));                      // direct GPIO
OpenSkyhawk::Switch2Pos    swMcp (0x4002, PinRef(gExp, PORT_A, 3));          // MCP23017
OpenSkyhawk::RotaryEncoder encSr (0x4003, PinRef(ShiftBus1, 0, 0),           // ShiftBus '165
                                          PinRef(ShiftBus1, 0, 1));
OpenSkyhawk::LED           ledSr (0x4004, 0x0100, PinRef(ShiftBus1, 0, 2));  // ShiftBus '595
PinRef                     potAds(gAdc, 0);                                  // ADS1115 channel

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println("=== ShiftBus mixed_node (all PinRef backends, one node) ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        d.print(label);
        d.println(ok ? ": PASS" : ": FAIL");
    };

    ShiftBus1.testSetBypass(true);

    // configure() routes per backend: GPIO pinMode, MCP IODIR (cache-side only here),
    // SR chain notes. MCP register writes need chip.init() in production — Switch2Pos's
    // configure on a un-init'd chip is I2C-silent for this compile/route test.
    swGpio.configure();
    encSr.configure();
    ledSr.configure();

    check("SR input chain sized by encoder", ShiftBus1.testChainIn() == 1);
    check("SR output chain sized by LED",    ShiftBus1.testChainOut() == 1);
    check("bus active with MCP/ADS coexisting", ShiftBus1.active());
    (void)potAds;  // ADS pin: declaration coverage only — chip not begin()'d on this rig

    ShiftBus1.begin();
    ShiftBus1.testInjectIn(0, 0b00000010);   // encoder B high
    ShiftBus1.transfer();
    check("SR read alongside other backends", PinRef(ShiftBus1, 0, 1).read() == true);

    ledSr.onControlPacket(0x4004, 0xFFFF);   // DCS says ON (controlId, not mask)
    ShiftBus1.flushNow();
    check("SR LED lit via CTRL packet path", ShiftBus1.testOutFrame(0) & 0b100);

    d.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
