// PanelGroup — interrupt dispatch / MCP cache test
//
// Verifies: PanelGroup::readCachedPin() returns the cached value after
// PanelGroup::setup() initialises the expander in polling-fallback mode
// (NO_INT_PIN). Polling fallback reads live ports every 20 ms.
//
// Hardware: STM32 + MCP23017 on I2C1 remap (PB8=SCL, PB9=SDA, addr 0x20).
// Connect a switch between GPA0 and GND (active-low per project standard).
// 10 kΩ pull-up to 3.3V on GPA0 required on the breakout board.
//
// Because interrupt_dispatch exercises the MCP23017 via real I2C, this test
// is hardware-in-the-loop only. Expected serial output after 2 seconds:
//   initial cache bit GPA0=1 (pull-up, switch open)
//   after toggling switch closed: cache bit GPA0=0

#include <Arduino.h>
#include <STM32Board.h>
#include <Wire.h>
#include <MCP23017.h>
#include <PanelGroup.h>

// MCP23017 at address 0x20 on Wire (I2C1)
MCP23017 gExpander(0x20, Wire);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== PanelGroup interrupt_dispatch ===");
    STM32Board::diagSerial().println("Hardware: MCP23017 @ 0x20 required");

    Wire.setSDA(PB9);
    Wire.setSCL(PB8);
    Wire.begin(); // Bench workaround: production PCB uses Wire.begin() default (PB6/PB7).

    // Register in polling-fallback mode (no interrupt pin required)
    PanelGroup::registerExpander(gExpander);

    // setup() initialises the chip, reads baseline, arms heartbeat timer
    PanelGroup::setup();

    // GPA0 starts HIGH (switch open, 10 kΩ pull-up)
    bool initial = PanelGroup::readCachedPin(gExpander, 0 /*PORT_A*/, 0 /*bit*/);
    STM32Board::diagSerial().print("GPA0 initial cache: ");
    STM32Board::diagSerial().println(initial ? "HIGH (PASS — switch open)" : "LOW (FAIL or switch already closed)");
}

void loop() {
    PanelGroup::loop(); // drives polling fallback every 20 ms

    static uint32_t lastPrintMs = 0;
    uint32_t now = millis();
    if (now - lastPrintMs >= 500) {
        lastPrintMs = now;
        bool gpa0 = PanelGroup::readCachedPin(gExpander, 0, 0);
        bool gpa1 = PanelGroup::readCachedPin(gExpander, 0, 1);
        STM32Board::diagSerial().print("GPA0="); STM32Board::diagSerial().print(gpa0);
        STM32Board::diagSerial().print("  GPA1="); STM32Board::diagSerial().println(gpa1);
        STM32Board::diagSerial().println(gpa0 ? "Switch open (HIGH)" : "Switch CLOSED (LOW) — PASS");
    }
}
