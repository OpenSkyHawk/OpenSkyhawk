/**
 * @file PduCalStore.h
 * @brief PDU per-board calibration in a reserved flash page — append-only log, newest valid wins (#216).
 *
 * The F103 has no EEPROM, so calibration lives in the last 1 KB flash page (PDU_CAL_PAGE_ADDR),
 * outside the firmware image: the PDU builds set `board_upload.maximum_size = 64512` so the build
 * fails before code can grow into it, and an OpenOCD upload erases only the sectors it writes, so
 * a reflash keeps the calibration.
 *
 * The page holds up to 32 fixed 32-byte records. save() writes the next empty slot; load() returns
 * the last slot whose magic and CRC check out. The page is erased only when it is full, so a power
 * cut can lose at most the record being written — never the previous good one — except during the
 * rare full-page erase.
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>
#include "PduConfig.h"
#include "PduMath.h"

namespace OpenSkyhawk {
namespace Pdu {

/** @brief CalRecord.calFlags bits — which calibrations this record holds. */
enum CalFlag : uint8_t {
    CAL_VDDA = 0x01,  ///< vrefintMv anchored by CAL VDDA (DMM reading of the 3V3 rail)
    CAL_ZERO = 0x02,  ///< current zero offsets measured by CAL ZERO (no load)
    CAL_GAIN = 0x04,  ///< gain trims set (optional bench calibration)
    CAL_NTC  = 0x08,  ///< NTC one-point offset set (optional)
};

/** @brief One stored calibration — 32 bytes, little-endian, CRC-32 over bytes 0–27. */
struct __attribute__((packed)) CalRecord {
    uint32_t magic;        ///< 0x00: CAL_MAGIC
    uint8_t  version;      ///< 0x04: CAL_VERSION
    uint8_t  calFlags;     ///< 0x05: CalFlag bits
    uint16_t vrefintMv;    ///< 0x06: this chip's Vrefint, mV (0 = uncalibrated)
    int16_t  zero12DmA;    ///< 0x08: 12 V current zero offset, 0.1 mA
    int16_t  zero5DmA;     ///< 0x0A: 5 V current zero offset, 0.1 mA
    uint16_t iGain12;      ///< 0x0C: 12 V current gain trim, 10000 = ×1
    uint16_t iGain5;       ///< 0x0E: 5 V current gain trim
    uint16_t vGain12;      ///< 0x10: 12 V voltage gain trim
    uint16_t vGain5;       ///< 0x12: 5 V voltage gain trim
    int16_t  ntcOffsetDC;  ///< 0x14: NTC offset, 0.1 °C
    uint16_t rsvd;         ///< 0x16: reserved, 0
    uint32_t seq;          ///< 0x18: write counter (diagnostic)
    uint32_t crc;          ///< 0x1C: CRC-32 over bytes 0x00–0x1B
};
static_assert(sizeof(CalRecord) == 32, "CalRecord must be 32 bytes");

static constexpr uint32_t CAL_MAGIC   = 0x4C414350;  ///< 'PCAL' little-endian
static constexpr uint8_t  CAL_VERSION = 1;
static constexpr uint32_t CAL_PAGE_SIZE = 1024;
static constexpr uint8_t  CAL_SLOTS   = CAL_PAGE_SIZE / sizeof(CalRecord);  // 32

/** @brief An uncalibrated record: unity gains, zero offsets, no flags. */
CalRecord calDefaults();
/** @brief Fill magic/version/crc for `rec` (seq left as set). */
void calSeal(CalRecord& rec);
/** @brief True when `rec` has the right magic, version and CRC. */
bool calValid(const CalRecord& rec);
/** @brief The conversion values a record carries. */
CalValues calValues(const CalRecord& rec);

/** @brief The reserved flash page holding the calibration log. */
class CalStore {
public:
    explicit CalStore(uint32_t pageAddr = PDU_CAL_PAGE_ADDR) : _page(pageAddr) {}

    /** @brief Newest valid record into `out`. @return false (out = calDefaults()) when none. */
    bool load(CalRecord& out) const;

    /**
     * @brief Append `rec` (sealed here, seq = previous + 1); erases the page first when full.
     * @return true when the written slot reads back valid.
     */
    bool save(CalRecord& rec);

    /** @brief Erase the whole page — back to uncalibrated. */
    bool erase();

    /** @brief Slots holding a record (valid or not) — 0..CAL_SLOTS. */
    uint8_t usedSlots() const;

private:
    uint32_t _page;
    const CalRecord* slot(uint8_t i) const {
        return reinterpret_cast<const CalRecord*>(_page + (uint32_t)i * sizeof(CalRecord));
    }
};

} // namespace Pdu
} // namespace OpenSkyhawk
