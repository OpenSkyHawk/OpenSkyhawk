// Switch2Pos — configure() test
//
// Verifies:
//   configure() does not emit an EVT.
//   poll() is a no-op before forceReport() is called (_initialized == false).
//   Multiple poll() calls after configure() produce zero EVTs.
//
// Hardware: STM32. No jumper wires required for this test.
// PA0 configured as INPUT (floating) — poll() returns immediately because
// _initialized is false, so the pin is never read.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/Switch2Pos/Switch2Pos.h>

static constexpr uint16_t CTRL_ID  = 0xABCD;
static constexpr uint8_t  PIN_SW   = PA0;

static uint8_t gEvtCount = 0;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId == canIdEvt(NODE_ID)) gEvtCount++;
}

OpenSkyhawk::Switch2Pos gSw(CTRL_ID, PinRef(PIN_SW));

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== Switch2Pos configure ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptId(canIdEvt(NODE_ID));
    CANProtocol::startLoopback();

    // 1 entry registered at global scope
    uint8_t inputCount = 0;
    for (auto* p = OpenSkyhawk::InputBase::head(); p; p = p->next()) inputCount++;
    check("1 input registered", inputCount == 1);

    // configure() — sets pin INPUT, emits nothing
    gSw.configure();

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    delay(2);
    CANProtocol::drain();
    check("no EVT after configure()", gEvtCount == 0);

    // poll() must be a no-op before forceReport()
    for (int i = 0; i < 10; i++) {
        gSw.poll();
    }
    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    delay(2);
    CANProtocol::drain();
    check("no EVT after 10x poll() without forceReport()", gEvtCount == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
