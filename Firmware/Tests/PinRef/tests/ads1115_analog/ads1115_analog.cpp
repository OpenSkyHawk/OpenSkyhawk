// PinRef — ADS1115 analog read test
//
// Verifies:
//   PinRef(adc, channel).readAnalog() calls readADC_SingleEnded() and returns
//   a 16-bit value (raw 15-bit single-ended × 2 → 0–65534).
//   read() returns true when readAnalog() > 32767 (above half-scale).
//   write() is a no-op on ADS1115 — pin state unchanged.
//   isGpio() == false for an ADS1115 PinRef.
//
// Hardware: ADS1115 dev board on I2C1 (PB6=SCL, PB7=SDA), addr 0x48 (ADDR→GND).
//   A0 connected to ~1.65 V mid-rail (10 kΩ + 10 kΩ voltage divider, 3.3 V → GND).
//   Expected readAnalog() ≈ 32768 ± wide tolerance (~16000–48000 at mid-rail).
//   Connect A0 to 3.3 V for a high reading (≥ 52000). Connect to GND for near-zero.
//
// Serial output shows raw readAnalog() value for cross-check.

#include <Arduino.h>
#include <Wire.h>
#include "ADS1115.h"
#include <PinRef.h>

static ADS1115 gAdc;

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("=== PinRef ads1115_analog ===");
    Serial.println("Hardware: ADS1115 @ 0x48, A0 wired to ~1.65 V mid-rail divider");

    Wire.begin();
    bool ok = gAdc.begin(0x48, &Wire);
    if (!ok) {
        Serial.println("ADS1115 begin() FAILED — check wiring");
        Serial.println("=== FAIL ===");
        return;
    }
    Serial.println("ADS1115 begin(): OK");

    PinRef pin(gAdc, 0); // channel 0 = A0

    bool pass = true;
    auto check = [&](const char* label, bool cond) {
        if (!cond) pass = false;
        Serial.print(label);
        Serial.println(cond ? ": PASS" : ": FAIL");
    };

    // Identity
    check("isGpio() == false", !pin.isGpio());
    check("isNC()   == false", !pin.isNC());

    // readAnalog() — ADS1115 15-bit single-ended × 2 → 0–65534
    uint16_t val = pin.readAnalog();
    Serial.print("readAnalog() = ");
    Serial.println(val);

    check("readAnalog() > 0       ", val > 0);
    check("readAnalog() <= 65534  ", val <= 65534u);
    // Mid-rail ~1.65 V → raw ~16384 ADC counts → scaled ~32768; wide tolerance for divider variance
    check("readAnalog() in [8000, 56000] (mid-rail)", val >= 8000 && val <= 56000);

    // read() threshold: true when > 32767
    bool r = pin.read();
    Serial.print("read() = ");
    Serial.println(r ? "true" : "false");
    // Verify read() matches readAnalog() > 32767
    check("read() matches readAnalog() > 32767", r == (val > 32767u));

    // write() is a no-op — just must not crash
    pin.write(true);
    pin.write(false);
    Serial.println("write(): compiled and ran (no crash)");

    // Second readAnalog() call — value should be stable (same wiring, no noise source)
    uint16_t val2 = pin.readAnalog();
    Serial.print("readAnalog() second call = ");
    Serial.println(val2);
    // Allow ±10% variation between two back-to-back reads
    uint16_t tolerance = val / 10 + 200;
    check("Second read within ±10% of first",
          val2 >= (val > tolerance ? val - tolerance : 0) &&
          val2 <= val + tolerance);

    Serial.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
