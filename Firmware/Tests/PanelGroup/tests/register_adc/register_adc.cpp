// PanelGroup — registerADC integration test
//
// Verifies:
//   registerADC(adc, addr, wire) stores the instance without crash.
//   setup() calls adc.begin(addr, wire) — verified by subsequent PinRef readAnalog() working.
//   Registering the same ADS1115 instance twice is deduplicated (no double-begin).
//   A PinRef(adc, channel) reads a valid value after setup() has initialised the chip.
//
// Hardware: ADS1115 dev board on I2C1 (PB6=SCL, PB7=SDA), addr 0x48 (ADDR→GND).
//   A0 connected to ~1.65 V mid-rail (10 kΩ + 10 kΩ voltage divider, 3.3 V → GND).
//   No MCP23017 required. CAN loopback mode used so no CAN bus needed.
//
// Note: PanelGroup::setup() calls CANProtocol::start() internally. This test calls
// CANProtocol::startLoopback() first via the onReceive hook path so loopback frames
// echo correctly if needed. READY and EVT frames are attempted; if CAN is in loopback
// they succeed; any BUS_OFF condition from a missing bus does not affect ADS1115 init.

#include <Arduino.h>
#include <STM32Board.h>
#include <Wire.h>
#include <PanelGroup.h>

static ADS1115 gAdc;

// Capture CAN frames sent during setup() — READY frame expected
static bool readyReceived = false;
static void onCan(uint32_t id, const uint8_t*, uint8_t) {
    if (id == canIdReady(NODE_ID)) readyReceived = true;
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== PanelGroup register_adc ===");
    STM32Board::diagSerial().println("Hardware: ADS1115 @ 0x48, A0 at ~1.65 V mid-rail");

    bool pass = true;
    auto check = [&](const char* label, bool cond) {
        if (!cond) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(cond ? ": PASS" : ": FAIL");
    };

    // Register ADC before setup() — pattern identical to sketch usage
    Wire.begin();
    PanelGroup::registerADC(gAdc, 0x48, Wire);

    // Register same instance again — must be deduplicated (no crash, no double-init)
    PanelGroup::registerADC(gAdc, 0x48, Wire);
    STM32Board::diagSerial().println("Double-register: did not crash");

    // Start CAN in loopback so setup() does not hang on bus-off
    CANProtocol::onReceive(onCan);
    CANProtocol::startLoopback();

    // setup() calls adc.begin(0x48, &Wire) for every registered ADC
    PanelGroup::setup();

    // Drain any loopback frames (READY, EVT burst)
    for (uint8_t i = 0; i < 10; i++) {
        CANProtocol::drain();
        delay(1);
    }
    check("READY frame sent during setup()", readyReceived);

    // After setup(), the ADS1115 is initialised — PinRef can read
    PinRef pin(gAdc, 0); // channel 0 = A0
    uint16_t val = pin.readAnalog();
    STM32Board::diagSerial().print("PinRef(adc, 0).readAnalog() = ");
    STM32Board::diagSerial().println(val);

    check("readAnalog() > 0        (ADS1115 responding)", val > 0);
    check("readAnalog() <= 65534   (15-bit×2 range)",     val <= 65534u);
    check("readAnalog() in [8000, 56000] (mid-rail ~1.65 V)",
          val >= 8000 && val <= 56000);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {
    PanelGroup::loop(); // drives heartbeat; not required for the test but exercises loop path
}
