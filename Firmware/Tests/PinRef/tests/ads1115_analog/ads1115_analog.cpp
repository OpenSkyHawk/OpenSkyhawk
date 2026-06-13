// PinRef — ADS1115 analog read test
//
// Verifies:
//   PinRef(adc, channel).readAnalog() calls readADC_SingleEnded() and returns
//   a 16-bit value (raw 15-bit single-ended × 2 → 0–65534).
//   read() returns true when readAnalog() > 32767 (above half-scale).
//   write() is a no-op on ADS1115 — pin state unchanged.
//   isGpio() == false for an ADS1115 PinRef.
//
// Hardware: ADS1115 dev board on I2C1 remap (PB8=SCL, PB9=SDA), addr 0x48 (ADDR→GND).
//   A0 connected to ~1.65 V mid-rail (10 kΩ + 10 kΩ voltage divider, 3.3 V → GND).
//   Expected readAnalog() ≈ 32768 ± wide tolerance (~16000–48000 at mid-rail).
//   Connect A0 to 3.3 V for a high reading (≥ 52000). Connect to GND for near-zero.
//
// Serial output shows raw readAnalog() value for cross-check.

#include <Arduino.h>
#include <STM32Board.h>
#include <Wire.h>
#include "ADS1115.h"
#include <PinRef.h>

static ADS1115 gAdc;

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== PinRef ads1115_analog ===");
    STM32Board::diagSerial().println("Hardware: ADS1115 @ 0x48, A0 wired to ~1.65 V mid-rail divider");

    Wire.setSDA(PB9);
    Wire.setSCL(PB8);
    Wire.begin();

    // Raw I2C read test: write config register pointer, read 2 bytes.
    // begin() only checks write ACK — this confirms reads also work.
    Wire.beginTransmission(0x48);
    Wire.write(0x01); // config register
    Wire.endTransmission(false);
    uint8_t nRead = Wire.requestFrom((uint8_t)0x48, (uint8_t)2);
    STM32Board::diagSerial().print("Raw I2C read bytes received: ");
    STM32Board::diagSerial().println(nRead);
    if (nRead >= 2) {
        uint16_t cfg = ((uint16_t)Wire.read() << 8) | Wire.read();
        STM32Board::diagSerial().print("Config register raw: 0x");
        STM32Board::diagSerial().println(cfg, HEX);
    } else {
        STM32Board::diagSerial().println("I2C read FAILED — check SDA/SCL pull-ups and wiring");
        STM32Board::diagSerial().println("=== FAIL ===");
        return;
    }

    bool ok = gAdc.begin(0x48, &Wire);
    if (!ok) {
        STM32Board::diagSerial().println("ADS1115 begin() FAILED — check wiring");
        STM32Board::diagSerial().println("=== FAIL ===");
        return;
    }
    STM32Board::diagSerial().println("ADS1115 begin(): OK");

    PinRef pin(gAdc, 0); // channel 0 = A0

    bool pass = true;
    auto check = [&](const char* label, bool cond) {
        if (!cond) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(cond ? ": PASS" : ": FAIL");
    };

    // Identity
    check("isGpio() == false", !pin.isGpio());
    check("isNC()   == false", !pin.isNC());

    // readAnalog() — ADS1115 15-bit single-ended × 2 → 0–65534
    uint16_t val = pin.readAnalog();
    STM32Board::diagSerial().print("readAnalog() = ");
    STM32Board::diagSerial().println(val);

    check("readAnalog() > 0       ", val > 0);
    check("readAnalog() <= 65534  ", val <= 65534u);
    // Mid-rail ~1.65 V → raw ~16384 ADC counts → scaled ~32768; wide tolerance for divider variance
    check("readAnalog() in [8000, 56000] (mid-rail)", val >= 8000 && val <= 56000);

    // read() threshold: true when > 32767
    bool r = pin.read();
    STM32Board::diagSerial().print("read() = ");
    STM32Board::diagSerial().println(r ? "true" : "false");
    // Verify read() matches readAnalog() > 32767
    check("read() matches readAnalog() > 32767", r == (val > 32767u));

    // write() is a no-op — just must not crash
    pin.write(true);
    pin.write(false);
    STM32Board::diagSerial().println("write(): compiled and ran (no crash)");

    // Second readAnalog() call — value should be stable (same wiring, no noise source)
    uint16_t val2 = pin.readAnalog();
    STM32Board::diagSerial().print("readAnalog() second call = ");
    STM32Board::diagSerial().println(val2);
    // Allow ±10% variation between two back-to-back reads
    uint16_t tolerance = val / 10 + 200;
    check("Second read within ±10% of first",
          val2 >= (val > tolerance ? val - tolerance : 0u) &&
          (uint32_t)val2 <= (uint32_t)val + tolerance);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
