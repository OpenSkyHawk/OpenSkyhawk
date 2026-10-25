// PowerMonitor — PduCalStore on the real reserved flash page (0x0800FC00).
//
// Erases the page, saves / reloads records, plants a corrupt record (bad CRC) that load() must
// skip, fills the page to force the wrap-around erase, and erases again at the end — the board is
// left uncalibrated. Ends with "=== cal_store ALL PASS (n) ===".

#include "../common/check.h"
#include <PduCalStore.h>
#include <string.h>

using namespace OpenSkyhawk::Pdu;

static CalStore store;

// Program a 32-byte record verbatim (no sealing) into slot `i` — to plant a corrupt one.
static void programRaw(uint8_t i, const CalRecord& r) {
    uint32_t w[sizeof(CalRecord) / 4];
    memcpy(w, &r, sizeof(w));
    HAL_FLASH_Unlock();
    for (size_t k = 0; k < sizeof(w) / 4; k++)
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, PDU_CAL_PAGE_ADDR + i * sizeof(CalRecord) + k * 4, w[k]);
    HAL_FLASH_Lock();
}

void setup() {
    begin("cal_store");
    CalRecord r;

    check("erase", store.erase());
    checkEq("empty page: 0 slots used", store.usedSlots(), 0);
    check("empty page: load() false", !store.load(r));
    checkEq("empty page: defaults unity gain", r.iGain12, GAIN_UNITY);
    check("defaults are valid records", calValid(r));

    CalRecord a = calDefaults();
    a.vrefintMv = 1210; a.zero12DmA = 123; a.calFlags = CAL_VDDA;
    check("save #1", store.save(a));
    checkEq("1 slot used", store.usedSlots(), 1);
    check("load #1", store.load(r));
    checkEq("vrefint round-trip", r.vrefintMv, 1210);
    checkEq("zero12 round-trip", r.zero12DmA, 123);
    checkEq("seq 1", (long)r.seq, 1);
    checkEq("calValues: VDDA flag -> vrefint used", calValues(r).vrefintMv, 1210);

    CalRecord b = r;
    b.zero5DmA = -45; b.calFlags |= CAL_ZERO;
    check("save #2", store.save(b));
    store.load(r);
    checkEq("newest wins: zero5", r.zero5DmA, -45);
    checkEq("seq 2", (long)r.seq, 2);

    // A corrupt record after the good ones must be skipped, not trusted.
    CalRecord bad = r;
    bad.vrefintMv = 999;          // CRC no longer matches
    programRaw(2, bad);
    checkEq("corrupt slot counted as used", store.usedSlots(), 3);
    store.load(r);
    checkEq("corrupt record skipped -> previous good", r.vrefintMv, 1210);
    checkEq("still seq 2", (long)r.seq, 2);

    // Fill to 32 slots, then one more save wraps: page erased, record lands in slot 0.
    while (store.usedSlots() < CAL_SLOTS) store.save(b);
    checkEq("page full", store.usedSlots(), CAL_SLOTS);
    CalRecord before;
    store.load(before);
    check("save on a full page", store.save(b));
    checkEq("wrapped: 1 slot used", store.usedSlots(), 1);
    store.load(r);
    checkEq("wrapped: seq continues", (long)r.seq, (long)before.seq + 1);
    checkEq("wrapped: data kept", r.zero5DmA, -45);

    check("final erase", store.erase());
    check("left uncalibrated", !store.load(r));

    summary("cal_store");
}

void loop() {}
