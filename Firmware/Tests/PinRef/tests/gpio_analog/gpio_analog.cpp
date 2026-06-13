// PinRef — GPIO analog read test
//
// Hardware: PA1 connected to a known voltage via resistor divider.
// Verifies: readAnalog() returns a 16-bit value via analogReadResolution(16) set in
// STM32Board::begin(). STM32duino scales 12-bit ADC (0–4095) → 0–65520 internally.
// Expected range for ~1.65 V on a 3.3 V reference: ~26000–36000.
//
// Scale verification: confirm value ≤ 65520 (4095 << 4 = 65520 — framework ceiling).

#include <Arduino.h>
#include <STM32Board.h>
#include <PinRef.h>

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== PinRef gpio_analog ===");
    STM32Board::diagSerial().println("Hardware: PA1 connected to ~1.65 V (mid-rail divider).");

    PinRef adc(PA1);
    uint16_t val    = adc.readAnalog();
    uint16_t rawAdc = analogRead(PA1);  // 0–65520 — analogReadResolution(16) set in STM32Board::begin()

    STM32Board::diagSerial().print("PA1 analogRead() 16-bit = ");
    STM32Board::diagSerial().println(rawAdc);
    STM32Board::diagSerial().print("PA1 readAnalog() scaled = ");
    STM32Board::diagSerial().println(val);

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    // Any non-zero reading indicates the pin is connected and the ADC is working
    check("readAnalog() > 0          ", val > 0);
    // Max 12-bit value (4095) × 16 = 65520; 65535 would indicate wrong scaling
    check("readAnalog() <= 65520     ", val <= 65520);
    // For ~1.65 V mid-rail: raw ADC ~2048, scaled ~32768 ± wide tolerance for divider variation
    check("readAnalog() in [8000,56000]", val >= 8000 && val <= 56000);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
