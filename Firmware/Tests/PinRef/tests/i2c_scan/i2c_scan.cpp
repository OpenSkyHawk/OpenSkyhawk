// I2C bus scanner — diagnostic only, not a formal test.
//
// Sequentially scans all three valid I2C pin combos on STM32F103:
//   Config A: I2C1 default  — SCL=PB6,  SDA=PB7
//   Config B: I2C1 remap    — SCL=PB8,  SDA=PB9
//   Config C: I2C2          — SCL=PB10, SDA=PB11
// Reinitialises Wire between configs using end()/begin().
//
// Expected addresses: 0x20 MCP23017, 0x3C/0x3D OLED, 0x48 ADS1115

#include <Arduino.h>
#include <STM32Board.h>
#include <Wire.h>

static void scanConfig(uint32_t sda, uint32_t scl, const char* label) {
    Wire.end();
    Wire.setSDA(sda);
    Wire.setSCL(scl);
    Wire.begin();
    delay(10);

    STM32Board::diagSerial().print("  [");
    STM32Board::diagSerial().print(label);
    STM32Board::diagSerial().print("] ");
    uint8_t found = 0;
    uint8_t firstErr = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            STM32Board::diagSerial().print("0x");
            if (addr < 16) STM32Board::diagSerial().print("0");
            STM32Board::diagSerial().print(addr, HEX);
            STM32Board::diagSerial().print(" ");
            found++;
        } else if (firstErr == 0) {
            firstErr = err;
        }
    }
    if (found == 0) {
        STM32Board::diagSerial().print("none (err=");
        STM32Board::diagSerial().print(firstErr);
        STM32Board::diagSerial().println(")");
    } else {
        STM32Board::diagSerial().println();
    }
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    Wire.setSDA(PB9);
    Wire.setSCL(PB8);
    Wire.begin(); // must be called before first scanConfig() calls Wire.end()
    STM32Board::diagSerial().println("=== I2C Scanner — all pin configs ===");
    STM32Board::diagSerial().println("Scanning every 5 seconds...");
}

void loop() {
    STM32Board::diagSerial().println("--- scan ---");
    scanConfig(PB9, PB8, "B: I2C1 PB8(SCL)/PB9(SDA)");
    scanConfig(PB7, PB6, "A: I2C1 PB6(SCL)/PB7(SDA)");
    scanConfig(PB11,PB10,"C: I2C2 PB10(SCL)/PB11(SDA)");
    delay(5000);
}
