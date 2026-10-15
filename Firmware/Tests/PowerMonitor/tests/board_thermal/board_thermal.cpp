// PowerMonitor — BoardThermal thresholds, hysteresis and NTC fault (injected values).
//
// Defaults: warn 60.0 °C, critical 75.0 °C, hysteresis 3.0 °C. Ends with
// "=== board_thermal ALL PASS (n) ===".

#include "../common/check.h"
#include <BoardThermal.h>

using namespace OpenSkyhawk;

BoardThermal th;   // FaultSource: static lifetime

static bool has(PduStatusFlag f) { return th.flags() & (uint8_t)f; }

void setup() {
    begin("board_thermal");

    th.sample(900, false, /*armed=*/false);
    checkEq("blanked: no flags even at 90 C", th.flags(), 0);
    checkEq("blanked: temperature still reported", th.tempDC(), 900);

    th.sample(450, false, true);
    checkEq("45 C: healthy", th.flags(), 0);
    th.sample(600, false, true);
    check("60 C: warn", has(PduStatusFlag::TEMP_WARN));
    check("warn is not a node fault", th.faultCode() == NodeFaultCode::NONE);
    th.sample(575, false, true);
    check("57.5 C: warn held (hysteresis)", has(PduStatusFlag::TEMP_WARN));
    th.sample(570, false, true);
    check("57.0 C: warn cleared", !has(PduStatusFlag::TEMP_WARN));

    th.sample(750, false, true);
    check("75 C: critical", has(PduStatusFlag::TEMP_CRIT));
    check("critical -> OVER_TEMPERATURE", th.faultCode() == NodeFaultCode::OVER_TEMPERATURE);
    th.sample(725, false, true);
    check("72.5 C: critical held", has(PduStatusFlag::TEMP_CRIT));
    th.sample(700, false, true);
    check("70 C: critical cleared, warn kept", !has(PduStatusFlag::TEMP_CRIT) && has(PduStatusFlag::TEMP_WARN));

    th.sample(0, true, true);
    check("NTC fault flagged", has(PduStatusFlag::NTC_FAULT));
    check("NTC fault -> SENSOR_FAULT", th.faultCode() == NodeFaultCode::SENSOR_FAULT);
    checkEq("NTC fault -> temperature unavailable", th.tempDC(), INT16_MIN);
    check("NTC fault keeps last warn state", has(PduStatusFlag::TEMP_WARN));
    th.sample(400, false, true);
    checkEq("NTC back, 40 C: healthy", th.flags(), 0);

    summary("board_thermal");
}

void loop() {}
