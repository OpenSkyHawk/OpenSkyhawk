// PowerMonitor — RailMonitor flags, timing, fault priority and page packing (injected values).
//
// A simulated clock steps in 10 ms scans. Default thresholds (PduConfig.h): 12 V UV 11400 /
// OV 13200 / OC 4500, hysteresis 100 mV, set 50 ms (OC 100 ms), clear 500 ms, fuse-open < 1000 mV.
// Ends with "=== rail_monitor ALL PASS (n) ===".

#include "../common/check.h"
#include <RailMonitor.h>
#include <PduConfig.h>

using namespace OpenSkyhawk;

// Globals: FaultSource objects must have static lifetime.
RailMonitor m12({TelemPage::PDU_RAIL_12V, "12V", PDU_12V_UV_MV, PDU_12V_OV_MV, PDU_12V_OC_MA, true});
RailMonitor m5 ({TelemPage::PDU_RAIL_5V,  "5V",  PDU_5V_UV_MV,  PDU_5V_OV_MV,  PDU_5V_OC_MA,  false});
RailMonitor mp ({TelemPage::PDU_RAIL_12V, "12V", PDU_12V_UV_MV, PDU_12V_OV_MV, PDU_12V_OC_MA, true});

static uint32_t T = 0;   // simulated millis()

// Feed scans every 10 ms up to and including `until`.
static void run(RailMonitor& m, uint16_t mv, int32_t ma, uint32_t until, bool railedV = false, bool armed = true) {
    while (T <= until) { m.sample(mv, ma, railedV, T, armed); T += 10; }
}

static bool has(const RailMonitor& m, PduRailFlag f) { return m.flags() & (uint8_t)f; }

void setup() {
    begin("rail_monitor");

    // Boot blanking: a dead rail + huge current while unarmed raises nothing.
    run(m12, 500, 9000, 400, false, /*armed=*/false);
    checkEq("blanked: no flags", m12.flags(), 0);

    run(m12, 12000, 1000, 990);
    checkEq("healthy: no flags", m12.flags(), 0);
    check("healthy: fault NONE", m12.faultCode() == NodeFaultCode::NONE);

    // Under-voltage needs 50 ms to set.
    run(m12, 11000, 1000, 1040);
    check("UV not set at 40 ms", !has(m12, PduRailFlag::UNDER_V));
    run(m12, 11000, 1000, 1050);
    check("UV set at 50 ms", has(m12, PduRailFlag::UNDER_V));
    check("UV -> UNDER_VOLTAGE", m12.faultCode() == NodeFaultCode::UNDER_VOLTAGE);

    // Inside the hysteresis band (UV + < 100 mV) it never clears.
    run(m12, 11450, 1000, 2100);
    check("UV held inside hysteresis", has(m12, PduRailFlag::UNDER_V));
    // Clear needs > UV + 100 mV for 500 ms.
    const uint32_t c0 = T;
    run(m12, 12000, 1000, c0 + 490);
    check("UV still set at 490 ms good", has(m12, PduRailFlag::UNDER_V));
    run(m12, 12000, 1000, c0 + 500);
    check("UV cleared at 500 ms good", !has(m12, PduRailFlag::UNDER_V));

    // Over-current needs 100 ms; clears below 90 % for 500 ms.
    const uint32_t o0 = T;
    run(m12, 12000, 5000, o0 + 90);
    check("OC not set at 90 ms", !has(m12, PduRailFlag::OVER_CURRENT));
    run(m12, 12000, 5000, o0 + 100);
    check("OC set at 100 ms", has(m12, PduRailFlag::OVER_CURRENT));
    check("OC -> SHORT_CIRCUIT", m12.faultCode() == NodeFaultCode::SHORT_CIRCUIT);
    run(m12, 12000, 4200, T + 800);
    check("OC held above 90 %", has(m12, PduRailFlag::OVER_CURRENT));
    run(m12, 12000, 3000, T + 500);
    check("OC cleared", !has(m12, PduRailFlag::OVER_CURRENT));

    // Fuse open: rail ~0 V — UV and FUSE_OPEN both set; FUSE_OPEN wins the fault code.
    run(m12, 500, 0, T + 60);
    check("fuse open set", has(m12, PduRailFlag::FUSE_OPEN));
    check("UV also set", has(m12, PduRailFlag::UNDER_V));
    check("FUSE_OPEN outranks UV", m12.faultCode() == NodeFaultCode::FUSE_OPEN);
    run(m12, 12000, 1000, T + 600);
    checkEq("recovered: no flags", m12.flags(), 0);

    // Railed voltage channel -> SENSOR_FAULT (OV too, which outranks it).
    run(m12, 14190, 1000, T + 60, /*railedV=*/true);
    check("sensor fault set", has(m12, PduRailFlag::SENSOR_FAULT));
    check("OV outranks sensor fault", m12.faultCode() == NodeFaultCode::OVER_VOLTAGE);

    // 5 V rail has no fuse-open check: a dead 5 V rail powers the node down instead.
    T = 0;
    run(m5, 500, 0, 100);
    check("5V dead: UV set", has(m5, PduRailFlag::UNDER_V));
    check("5V dead: no FUSE_OPEN", !has(m5, PduRailFlag::FUSE_OPEN));

    // Page packing + window: avg 12000, min 11800, avg I 700, peak 1200.
    mp.sample(12000, 1000,  false, 0,  true);
    mp.sample(11800, 1200,  false, 10, true);
    mp.sample(12200, -100,  false, 20, true);
    TelemPayload p = mp.buildPage(/*resetWindow=*/true);
    const uint8_t want[8] = {0x01, 0x00, 0xE0, 0x2E, 0x18, 0x2E, 0xBC, 0x02};
    bool same = true;
    for (int i = 0; i < 8; i++) same &= reinterpret_cast<const uint8_t*>(&p)[i] == want[i];
    check("page bytes 01 00 E02E 182E BC02", same);
    checkEq("peak current 1200", mp.peakMa(), 1200);
    // After the periodic reset, an empty window reports the last sample.
    p = mp.buildPage(false);
    checkEq("empty window -> last vMv", p.body[1] | (p.body[2] << 8), 12200);
    checkEq("empty window -> last iMa", (int16_t)(p.body[5] | (p.body[6] << 8)), -100);
    mp.resetPeak();
    checkEq("peak reset", mp.peakMa(), 0);

    summary("rail_monitor");
}

void loop() {}
