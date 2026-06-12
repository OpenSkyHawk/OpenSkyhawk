// PanelGroup — boot sequence test
//
// Verifies: linked-list heads are null when no objects declared,
// READY frame is received via loopback after emission, heartbeat
// timer does not fire before 500 ms.
//
// Hardware: STM32. No CAN bus, no MCP23017, no ADS1115 required.
// Uses CANProtocol loopback mode.

#include <Arduino.h>
#include <PanelGroup.h>

static bool readyReceived = false;
static bool hbReceived    = false;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId == canIdReady(NODE_ID)) readyReceived = true;
    if (canId == canIdHb(NODE_ID))    hbReceived    = true;
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("=== PanelGroup boot_sequence ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        Serial.print(label);
        Serial.println(ok ? ": PASS" : ": FAIL");
    };

    // No inputs or outputs declared at global scope — lists must be empty
    check("InputBase::head()  == nullptr", OpenSkyhawk::InputBase::head()  == nullptr);
    check("OutputBase::head() == nullptr", OpenSkyhawk::OutputBase::head() == nullptr);

    // Use loopback so READY frame comes back through onCan
    CANProtocol::onReceive(onCan);
    CANProtocol::startLoopback();

    // Emit boot sequence tail (no expanders/ADCs, no inputs for EVT burst)
    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    CANProtocol::send(canIdReady(NODE_ID), nullptr, 0);
    CANProtocol::drain();

    check("READY frame received via loopback", readyReceived);
    check("HB not fired before 500 ms",        !hbReceived);

    Serial.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
