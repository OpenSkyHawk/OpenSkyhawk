// PowerMonitor — TelemSchedule: 1 frame/s rotation, edge sends, hold-off, no burst (injected clock).
// Ends with "=== telem_schedule ALL PASS (n) ===".

#include "../common/check.h"
#include <TelemSchedule.h>

using namespace OpenSkyhawk::Pdu;

static TelemSchedule s;
static uint8_t flags[TelemSchedule::PAGES] = {0, 0, 0};

// Advance 1 ms at a time from `from` to `to`; record every send (slot, periodic, t).
struct Send { int8_t slot; bool periodic; uint32_t t; };
static Send sends[64];
static int nSends = 0;

static void run(uint32_t from, uint32_t to) {
    for (uint32_t t = from; t <= to; t++) {
        bool periodic = false;
        const int8_t slot = s.next(t, flags, periodic);
        if (slot < 0) continue;
        if (nSends < 64) sends[nSends++] = {slot, periodic, t};
        s.sent((uint8_t)slot, flags[slot], t);
    }
}

void setup() {
    begin("telem_schedule");

    s.start(0);
    run(0, 3500);
    checkEq("3 periodic frames in 3.5 s", nSends, 3);
    check("rotation 12V -> 5V -> STATUS", sends[0].slot == 0 && sends[1].slot == 1 && sends[2].slot == 2);
    check("all periodic", sends[0].periodic && sends[1].periodic && sends[2].periodic);
    check("one per second", sends[0].t == 1000 && sends[1].t == 2000 && sends[2].t == 3000);

    // A flag change goes out at once as a non-periodic send.
    nSends = 0;
    flags[1] = 0x01;                       // 5 V under-voltage appears at 3600
    run(3600, 3600);
    check("edge sent immediately", nSends == 1 && sends[0].slot == 1 && !sends[0].periodic);

    // Within the 100 ms hold-off a further change waits, then goes out.
    nSends = 0;
    flags[1] = 0x03;
    run(3601, 3699);
    checkEq("change inside hold-off waits", nSends, 0);
    run(3700, 3700);
    check("sent when hold-off ends", nSends == 1 && sends[0].slot == 1);

    // Edges don't disturb the rotation: next periodic is still at 4000, slot 0.
    nSends = 0;
    run(3701, 4000);
    check("rotation unaffected by edges", nSends == 1 && sends[0].slot == 0 && sends[0].periodic && sends[0].t == 4000);

    // After a long stall the schedule resumes without bursting.
    nSends = 0;
    run(9000, 9000);
    checkEq("stall: one frame, no burst", nSends, 1);
    run(9001, 9999);
    checkEq("stall: next frame a full period later", nSends, 1);
    run(10000, 10000);
    checkEq("stall: then back on cadence", nSends, 2);

    // Baseline is all-clear: a page that starts non-zero is edge-sent right after start().
    TelemSchedule s2;
    s2.start(0);
    const uint8_t f2[TelemSchedule::PAGES] = {0, 0, 0x03};
    bool periodic = true;
    const int8_t slot = s2.next(0, f2, periodic);
    check("non-zero at boot -> edge at once", slot == 2 && !periodic);

    summary("telem_schedule");
}

void loop() {}
