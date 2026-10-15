#include "PduCalStore.h"
#include <string.h>

#ifdef ARDUINO_ARCH_STM32
#include <Arduino.h>   // STM32 HAL flash API
#endif

namespace OpenSkyhawk {
namespace Pdu {

CalRecord calDefaults() {
    CalRecord r;
    memset(&r, 0, sizeof(r));
    r.iGain12 = r.iGain5 = r.vGain12 = r.vGain5 = GAIN_UNITY;
    calSeal(r);
    return r;
}

void calSeal(CalRecord& rec) {
    rec.magic   = CAL_MAGIC;
    rec.version = CAL_VERSION;
    rec.rsvd    = 0;
    rec.crc     = crc32(reinterpret_cast<const uint8_t*>(&rec), offsetof(CalRecord, crc));
}

bool calValid(const CalRecord& rec) {
    return rec.magic == CAL_MAGIC && rec.version == CAL_VERSION &&
           rec.crc == crc32(reinterpret_cast<const uint8_t*>(&rec), offsetof(CalRecord, crc));
}

CalValues calValues(const CalRecord& rec) {
    CalValues v;
    v.vrefintMv   = (rec.calFlags & CAL_VDDA) ? rec.vrefintMv : 0;
    v.zero12DmA   = rec.zero12DmA;
    v.zero5DmA    = rec.zero5DmA;
    v.iGain12     = rec.iGain12;
    v.iGain5      = rec.iGain5;
    v.vGain12     = rec.vGain12;
    v.vGain5      = rec.vGain5;
    v.ntcOffsetDC = rec.ntcOffsetDC;
    return v;
}

static bool slotEmpty(const CalRecord* s) {
    uint32_t w[sizeof(CalRecord) / 4];
    memcpy(w, s, sizeof(w));   // CalRecord is packed — copy rather than alias as uint32_t
    for (uint32_t x : w)
        if (x != 0xFFFFFFFFu) return false;
    return true;
}

uint8_t CalStore::usedSlots() const {
    uint8_t n = 0;
    while (n < CAL_SLOTS && !slotEmpty(slot(n))) n++;
    return n;
}

bool CalStore::load(CalRecord& out) const {
    const uint8_t used = usedSlots();
    for (int i = (int)used - 1; i >= 0; i--) {
        if (calValid(*slot((uint8_t)i))) { out = *slot((uint8_t)i); return true; }
    }
    out = calDefaults();
    return false;
}

#ifdef ARDUINO_ARCH_STM32

bool CalStore::erase() {
    FLASH_EraseInitTypeDef e = {};
    e.TypeErase   = FLASH_TYPEERASE_PAGES;
    e.PageAddress = _page;
    e.NbPages     = 1;
    uint32_t pageError = 0;
    HAL_FLASH_Unlock();
    const bool ok = HAL_FLASHEx_Erase(&e, &pageError) == HAL_OK;
    HAL_FLASH_Lock();
    return ok;
}

bool CalStore::save(CalRecord& rec) {
    CalRecord prev;
    rec.seq = load(prev) ? prev.seq + 1 : 1;
    calSeal(rec);

    uint8_t i = usedSlots();
    if (i >= CAL_SLOTS) {
        if (!erase()) return false;
        i = 0;
    }
    const uint32_t addr = _page + (uint32_t)i * sizeof(CalRecord);
    uint32_t w[sizeof(CalRecord) / 4];
    memcpy(w, &rec, sizeof(w));   // packed struct → aligned words for the word programmer
    HAL_FLASH_Unlock();
    bool ok = true;
    for (size_t k = 0; ok && k < sizeof(CalRecord) / 4; k++)
        ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + k * 4, w[k]) == HAL_OK;
    HAL_FLASH_Lock();
    return ok && calValid(*slot(i));
}

#else

bool CalStore::erase()              { return false; }
bool CalStore::save(CalRecord&)     { return false; }

#endif

} // namespace Pdu
} // namespace OpenSkyhawk
