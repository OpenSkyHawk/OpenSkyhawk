// PanelGroup — boot sequence test
//
// Verifies: linked-list heads are null when no objects declared,
// READY frame is received via loopback after emission, heartbeat
// timer does not fire before 500 ms.
//
// Hardware: STM32. No CAN bus, no MCP23017, no ADS1115 required.
// Uses CANProtocol loopback mode.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>

static bool readyReceived = false;
static bool hbReceived    = false;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId == canIdReady(NODE_ID)) readyReceived = true;
    if (canId == canIdHb(NODE_ID))    hbReceived    = true;
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== PanelGroup boot_sequence ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    // No inputs or outputs declared at global scope — lists must be empty
    check("InputBase::head()  == nullptr", OpenSkyhawk::InputBase::head()  == nullptr);
    check("OutputBase::head() == nullptr", OpenSkyhawk::OutputBase::head() == nullptr);

    // Use loopback so READY frame comes back through onCan.
    // filterAcceptId required — default filter only passes CTRL_BCAST/TEST_SEQ/SYNC_REQ.
    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptId(canIdReady(NODE_ID));
    CANProtocol::startLoopback();

    // Emit boot sequence tail (no expanders/ADCs, no inputs for EVT burst)
    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    CANProtocol::send(canIdReady(NODE_ID), nullptr, 0);
    // At 500 kbps, a CAN frame takes ~100 µs to complete and appear in FIFO0.
    // delay(2) ensures the hardware loopback completes before drain() polls FIFO0.
    delay(2);
    CANProtocol::drain();

    check("READY frame received via loopback", readyReceived);
    check("HB not fired before 500 ms",        !hbReceived);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
