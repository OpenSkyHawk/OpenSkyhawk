// CANProtocol — makeNodeHealthPayload + STM32Board internal-temp test (#213, nodeKind #344)
//
// Two parts:
//   1. Packing: makeNodeHealthPayload() copies nodeId/dieTempC through, puts the NodeKind in
//      byte 5, and zeroes flags + fault + reserved bytes 6-7 (no NODE_OVERHEAT_C in this build).
//   2. Live read: STM32Board::readDieTempC() returns a plausible value from the F103 internal
//      sensor (readVddMv() is read on-node for the temp calc; it is not transmitted).
//      UNCALIBRATED — this only sanity-checks the range, not accuracy.
//
// Expected output:
//   nodeId correct:          PASS
//   dieTempC correct:        PASS
//   flags == 0 (no trip):    PASS
//   fault/reserved zeroed:   PASS
//   nodeKind in byte 5:      PASS
//   kind UNKNOWN packs 0:    PASS
//   live temp in range:      PASS   (-40..125 °C, or sentinel if unavailable)
//   live Vdd in range:       PASS   (2700..3600 mV, or 0 if unavailable)
//   liveTemp=<n>C liveVdd=<n>mV

#include <STM32Board.h>
#include <CANProtocol.h>

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();  // sets analogReadResolution(16) — required before internal reads
    STM32Board::log("=== health_payload ===");

    const int8_t kTemp = 37;
    // healthy (fault defaults NONE), panel-group kind
    NodeHealthPayload p = CANProtocol::makeNodeHealthPayload(NODE_ID, NodeKind::PANEL_GROUP, kTemp);
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(&p);  // wire bytes, as sent on HEALTH_n

    // Faulted payload: faultId carries the code, DEGRADED flag derived, faultMask stays 0 (#163).
    NodeHealthPayload pf = CANProtocol::makeNodeHealthPayload(NODE_ID, NodeKind::PDU, kTemp,
                                                              NodeFaultCode::I2C_PERIPHERAL);
    // Pre-#344 equivalent: kind UNKNOWN is the 0 old firmware already sent in byte 5.
    NodeHealthPayload pu = CANProtocol::makeNodeHealthPayload(NODE_ID, NodeKind::UNKNOWN, kTemp);

    int8_t   liveTemp = STM32Board::readDieTempC();
    uint16_t liveVdd  = STM32Board::readVddMv();

    bool tempOk = (liveTemp == INT8_MIN) || (liveTemp >= -40 && liveTemp <= 125);
    bool vddOk  = (liveVdd == 0) || (liveVdd >= 2700 && liveVdd <= 3600);

    auto& d = STM32Board::diagSerial();
    d.println(F("--- health_payload results ---"));
    d.print(F("nodeId correct:          ")); d.println(p.nodeId   == NODE_ID ? F("PASS") : F("FAIL"));
    d.print(F("dieTempC correct:        ")); d.println(p.dieTempC == kTemp   ? F("PASS") : F("FAIL"));
    d.print(F("flags == 0 (no trip):    ")); d.println(p.flags    == 0       ? F("PASS") : F("FAIL"));
    d.print(F("fault/reserved zeroed:   "));
    d.println((p.faultMask | p.faultId | p.rsvd[0] | p.rsvd[1] | raw[6] | raw[7]) == 0 ? F("PASS") : F("FAIL"));
    d.print(F("nodeKind in byte 5:      "));
    d.println(raw[5] == (uint8_t)NodeKind::PANEL_GROUP && pf.nodeKind == (uint8_t)NodeKind::PDU
              ? F("PASS") : F("FAIL"));
    d.print(F("kind UNKNOWN packs 0:    ")); d.println(pu.nodeKind == 0 ? F("PASS") : F("FAIL"));
    d.print(F("fault: faultId set:      "));
    d.println(pf.faultId == (uint8_t)NodeFaultCode::I2C_PERIPHERAL ? F("PASS") : F("FAIL"));
    d.print(F("fault: DEGRADED flag:    "));
    d.println((pf.flags & (uint8_t)NodeHealthFlag::DEGRADED) ? F("PASS") : F("FAIL"));
    d.print(F("fault: faultMask stays 0:")); d.println(pf.faultMask == 0 ? F("PASS") : F("FAIL"));
    d.print(F("live temp in range:      ")); d.println(tempOk ? F("PASS") : F("FAIL"));
    d.print(F("live Vdd in range:       ")); d.println(vddOk  ? F("PASS") : F("FAIL"));

    d.print(F("liveTemp=")); d.print(liveTemp); d.print(F("C"));
    d.print(F(" liveVdd=")); d.print(liveVdd);  d.println(F("mV"));
}

void loop() {
    STM32Board::tick();
}
