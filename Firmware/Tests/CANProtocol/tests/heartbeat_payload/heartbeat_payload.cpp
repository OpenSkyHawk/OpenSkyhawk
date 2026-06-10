// CANProtocol — makeHeartbeatPayload test
//
// Verifies all fields are populated correctly:
//   nodeId  — matches the value passed in
//   rxCount — matches the value passed in
//   uptime  — > 0 after 1100 ms delay
//   flags   — 0x00 in loopback (no bus errors)
//   esr     — (CAN1->ESR >> 16); raw value printed for inspection
//
// Expected output:
//   nodeId correct:         PASS
//   rxCount correct:        PASS
//   uptime > 0:             PASS
//   flags == 0 (no errors): PASS
//   nodeId=1 uptime=1s rxCount=42 flags=0x00 esr=0x0000

#include <STM32Board.h>
#include <CANProtocol.h>

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== heartbeat_payload ===");

    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::filterAcceptAll();
    CANProtocol::startLoopback();

    delay(1100);  // let >1 second elapse so uptime rounds to >= 1

    const uint16_t kRxCount = 42;
    HeartbeatPayload p = CANProtocol::makeHeartbeatPayload(NODE_ID, kRxCount);

    auto& d = STM32Board::diagSerial();
    d.println(F("--- heartbeat_payload results ---"));
    d.print(F("nodeId correct:         ")); d.println(p.nodeId  == NODE_ID   ? F("PASS") : F("FAIL"));
    d.print(F("rxCount correct:        ")); d.println(p.rxCount == kRxCount  ? F("PASS") : F("FAIL"));
    d.print(F("uptime > 0:             ")); d.println(p.uptime  > 0          ? F("PASS") : F("FAIL"));
    d.print(F("flags == 0 (no errors): ")); d.println(p.flags   == 0         ? F("PASS") : F("FAIL"));

    d.print(F("nodeId="));    d.print(p.nodeId);
    d.print(F(" uptime="));   d.print(p.uptime);   d.print(F("s"));
    d.print(F(" rxCount="));  d.print(p.rxCount);
    d.print(F(" flags=0x"));  d.print(p.flags, HEX);
    d.print(F(" esr=0x"));    d.println(p.esr, HEX);
}

void loop() {
    STM32Board::tick();
}
