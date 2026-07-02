// ShiftBus — frame_map test (chip-less: SHIFTBUS_TEST bypass, no SPI, no shift registers)
//
// Validates the PinRef SR backend against the bus frames: direction-at-configure binds a pin
// to the '165 ('595) chain and auto-sizes it; writes land on the right outFrame bit and set
// dirty; injected inFrame bytes read back through PinRef::read(); readOutBit echoes writes;
// out-of-range chip/bit are safely ignored.
//
// Rig: bare STM32 (CAN not required — nothing emits).

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>

// Inputs on '165 chip 0 + chip 1; outputs on '595 chip 0 — the ARC/APN shape.
PinRef pinIn0 (ShiftBus1, 0, 3);   // '165 chip 0, D3
PinRef pinIn1 (ShiftBus1, 1, 6);   // '165 chip 1, D6 → chain grows to 2
PinRef pinOut0(ShiftBus1, 0, 0);   // '595 chip 0, Q0
PinRef pinOut1(ShiftBus1, 0, 5);   // '595 chip 0, Q5

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    auto& d = STM32Board::diagSerial();
    d.println("=== ShiftBus frame_map ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        d.print(label);
        d.println(ok ? ": PASS" : ": FAIL");
    };

    ShiftBus1.testSetBypass(true);

    // Direction-at-configure: inputs bind + size the '165 chain, outputs the '595 chain.
    pinIn0.configureAsInput();
    pinIn1.configureAsInput();
    pinOut0.configureAsOutput();
    pinOut1.configureAsOutput();
    check("chain auto-size in=2",  ShiftBus1.testChainIn()  == 2);
    check("chain auto-size out=1", ShiftBus1.testChainOut() == 1);
    check("bus active", ShiftBus1.active());

    ShiftBus1.begin();   // bypass: no SPI; primes the input cache from the injected frame

    // '165 read path: inject a frame, transfer, read through PinRef.
    ShiftBus1.testInjectIn(0, 0b00001000);   // chip 0: D3 high
    ShiftBus1.testInjectIn(1, 0b01000000);   // chip 1: D6 high
    ShiftBus1.transfer();
    check("in chip0 D3 reads high", pinIn0.read() == true);
    check("in chip1 D6 reads high", pinIn1.read() == true);

    ShiftBus1.testInjectIn(0, 0x00);
    ShiftBus1.transfer();
    check("in chip0 D3 reads low after clear", pinIn0.read() == false);
    check("in chip1 D6 still high", pinIn1.read() == true);

    // '595 write path: bits land in the frame, dirty tracks, read() echoes last write.
    check("not dirty before write", !ShiftBus1.dirty());
    pinOut0.write(true);
    pinOut1.write(true);
    check("dirty after write", ShiftBus1.dirty());
    check("writes stage only (committed frame still clear)", ShiftBus1.testOutFrame(0) == 0x00);
    check("out pin read() echoes write", pinOut0.read() == true && pinOut1.read() == true);

    uint16_t before = ShiftBus1.testTransferCount();
    ShiftBus1.flushNow();
    check("flushNow transfers + clears dirty",
          ShiftBus1.testTransferCount() == before + 1 && !ShiftBus1.dirty());
    check("flush commits Q0|Q5", ShiftBus1.testOutFrame(0) == 0b00100001);

    // Wire-order coverage: nIn=2 > nOut=1 → frame is 2 bytes, front-padded. wire[0] = pad
    // (falls off the far '595 end), wire[1] = out chip 0 (last byte shifted lands nearest).
    check("wire front-pad", ShiftBus1.testWire(0) == 0x00);
    check("wire chip0 last", ShiftBus1.testWire(1) == 0b00100001);

    // Stage/commit: writeDeferred stages (read() echoes it) but the committed frame — what
    // the ISR would ship — only changes at the next loop-context flush.
    pinOut1.writeDeferred(false);
    check("writeDeferred stages Q5 low",
          pinOut1.read() == false && ShiftBus1.dirty()
          && ShiftBus1.testOutFrame(0) == 0b00100001);
    ShiftBus1.flushNow();
    check("commit publishes stage",
          ShiftBus1.testOutFrame(0) == 0b00000001 && !ShiftBus1.dirty());

    // Guards: out-of-range chip/bit are no-ops, misuse reads false.
    PinRef bogus(ShiftBus1, OpenSkyhawk::ShiftBus::MAX_CHAIN, 9);
    bogus.configureAsOutput();
    bogus.write(true);
    check("out-of-range chip/bit ignored", ShiftBus1.testChainOut() == 1);
    check("readAnalog on SR = 0", pinIn0.readAnalog() == 0);

    d.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
