#include "AxisCal.h"

#include <string.h>

namespace OpenSkyhawk {

uint16_t calCrc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000u) ? (uint16_t)((uint16_t)(crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

bool axisCalValid(const AxisCal& cal) {
    return (uint32_t)cal.min    + cal.deadzone < cal.centre
        && (uint32_t)cal.centre + cal.deadzone < cal.max;
}

uint16_t axisCalApply(const AxisCal& cal, uint16_t raw) {
    if (!axisCalValid(cal)) return raw;   // uncalibrated — identity passthrough
    if (raw <= cal.min)     return 0;
    if (raw >= cal.max)     return 65535;

    // Lower segment maps [min, centre) onto [0, 32768); upper maps [centre, max) onto
    // [32768, 65535). Both divisors are >= 1 because axisCalValid passed. Note that the
    // degenerate cases (centre == min + 1, centre == max - 1) leave their segment with no
    // interior value at all, so neither branch can be entered with a zero numerator range.
    if (raw < cal.centre) {
        return (uint16_t)(((uint32_t)(raw - cal.min) * 32768u)
                          / (uint32_t)(cal.centre - cal.min));
    }
    return (uint16_t)(32768u + ((uint32_t)(raw - cal.centre) * 32767u)
                               / (uint32_t)(cal.max - cal.centre));
}

uint16_t calBlobCrc(const CalBlob& blob) {
    return calCrc16(reinterpret_cast<const uint8_t*>(&blob), offsetof(CalBlob, crc));
}

bool calBlobValid(const CalBlob& blob) {
    return blob.magic   == CAL_MAGIC
        && blob.version == CAL_VERSION
        && blob.crc     == calBlobCrc(blob);
}

void calBlobClear(CalBlob& blob) {
    memset(&blob, 0, sizeof(blob));
}

void calBlobSeal(CalBlob& blob) {
    blob.magic   = CAL_MAGIC;
    blob.version = CAL_VERSION;
    blob.crc     = calBlobCrc(blob);
}

// ── Calibration wire protocol ─────────────────────────────────────────────────

bool calLenValidForType(uint8_t type, uint16_t len) {
    switch (type) {
        case CAL_T_HELLO:
        case CAL_T_GET_CAL:
        case CAL_T_SESSION_CLOSE:
        case CAL_T_KEEPALIVE:        return len == 0;

        case CAL_T_SESSION_OPEN:
        case CAL_T_STREAM_SELECT:
        case CAL_T_RESET:
        case CAL_T_ACK:              return len == 1;

        case CAL_T_NACK:             return len == 3;
        case CAL_T_RAW:              return len == 5;
        case CAL_T_SESSION_ACK:      return len == 5;
        case CAL_T_HELLO_ACK:        return len == 6;
        case CAL_T_COMMIT:           return len == 9;   // exactly one axis, never a batch
        case CAL_T_CAL_DATA:         return len == 82;

        default:                     return false;   // unknown type is an error, not a skip
    }
}

uint16_t calBuildFrame(uint8_t* out, uint16_t outCap,
                       uint8_t type, uint8_t seq,
                       const uint8_t* payload, uint16_t len) {
    if (!out) return 0;
    if (!calLenValidForType(type, len)) return 0;
    if (len && !payload) return 0;

    const uint16_t total = (uint16_t)(CAL_ENVELOPE_BYTES + len);
    if (outCap < total) return 0;

    out[0] = CAL_FRAME_MAGIC[0];
    out[1] = CAL_FRAME_MAGIC[1];
    out[2] = CAL_FRAME_MAGIC[2];
    out[3] = CAL_FRAME_MAGIC[3];
    out[4] = type;
    out[5] = seq;
    out[6] = (uint8_t)(len & 0xFF);
    out[7] = (uint8_t)(len >> 8);
    for (uint16_t i = 0; i < len; ++i) out[8 + i] = payload[i];

    // Coverage starts at TYPE and ends at the last payload byte — magic excluded.
    const uint16_t crc = calCrc16(out + 4, (size_t)(4 + len));
    out[8 + len]       = (uint8_t)(crc & 0xFF);
    out[9 + len]       = (uint8_t)(crc >> 8);
    return total;
}

bool calFrameCrcOk(const uint8_t* frame, uint16_t n) {
    if (!frame || n < CAL_ENVELOPE_BYTES) return false;
    const uint16_t len = (uint16_t)(frame[6] | ((uint16_t)frame[7] << 8));
    if ((uint16_t)(CAL_ENVELOPE_BYTES + len) != n) return false;

    const uint16_t want = calCrc16(frame + 4, (size_t)(4 + len));
    const uint16_t got  = (uint16_t)(frame[8 + len] | ((uint16_t)frame[9 + len] << 8));
    return want == got;
}

} // namespace OpenSkyhawk
