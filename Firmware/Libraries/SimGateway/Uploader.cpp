// Gateway firmware uploader (#366). See Uploader.h; contract in FirmwarePlan/07 § Uploader.

#include "Uploader.h"
#include "AxisCal.h"         // CAL_T_UPL_*, CAL_NACK_* — the channel's type codes
#include "UploadProtocol.h"
#include <BootProtocol.h>    // the bootloader's frames and the relay encoding
#include <FirmwareImage.h>   // the identity block a staged image must carry
#include <NodeStatus.h>      // the _NODE_STATUS row the confirmation reads
#include <string.h>

using namespace OpenSkyhawk;
namespace B = OpenSkyhawk::Boot;

namespace {

Uploader::Io _io = {};

// ── Timing ────────────────────────────────────────────────────────────────────
constexpr uint32_t SESSION_IDLE_MS   = 30000;   // staging / staged, no client frame → session dropped
constexpr uint32_t CLIENT_GONE_MS    = 5000;    // no client frame → stop sending UPL_STATUS / UPL_RESULT
constexpr uint32_t STATUS_EVERY_MS   = 250;
constexpr uint32_t CONNECT_EVERY_MS  = 100;     // CONNECT while entering — also catches a reset board
constexpr uint32_t ENTER_TIMEOUT_MS  = 10000;
constexpr uint32_t REPLY_TIMEOUT_MS  = 3000;    // SET_NODE_ID / END / GO
constexpr uint32_t ERASE_TIMEOUT_MS  = 6000;    // BEGIN erases up to 124 pages, ~25 ms each
constexpr uint32_t ACK_TIMEOUT_MS    = 500;
constexpr uint8_t  MAX_STALLS        = 6;
constexpr uint32_t CONFIRM_TIMEOUT_MS = 10000;

// ── Staging ───────────────────────────────────────────────────────────────────
uint8_t  _img[UPL_MAX_IMAGE_BYTES];
uint32_t _len = 0, _crc = 0;
UplState _state = UPL_STATE_IDLE;
uint32_t _lastClientMs = 0;
const FirmwareImage::Identity* _id = reinterpret_cast<const FirmwareImage::Identity*>(_img + FirmwareImage::IDENTITY_OFFSET);

// ── What PanelBridge reports (_NODE_STATUS), per NODE_ID ──────────────────────
struct Seen { bool present; uint8_t bootloader, bootState, fw[3]; uint32_t ms; };
Seen _seen[64];

// ── The flash ─────────────────────────────────────────────────────────────────
UplPhase _phase;
uint8_t  _target, _newId, _attempt, _stalls;
uint16_t _cmdId, _rspId, _uid;
bool     _unconfigured, _uidKnown, _sentOnce;
uint32_t _phaseMs, _flashMs, _lastSendMs, _lastStatusMs, _ackDeadline, _confirmFromMs;
uint16_t _blocks, _acked, _sent, _goback;
uint8_t  _lastTarget = 0, _lastResult = UPL_RESULT_NONE, _lastFw[3] = {0, 0, 0};

inline void put16(uint8_t* p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
inline uint32_t get32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

bool clientHere(uint32_t now) { return now - _lastClientMs < CLIENT_GONE_MS; }

// CRC-32/MPEG-2 over little-endian words, the identity block's imageCrc word skipped (FirmwareImage.h).
uint32_t imageCrc(uint32_t len) {
    constexpr uint32_t skip = FirmwareImage::IDENTITY_OFFSET + FirmwareImage::OFF_CRC;
    uint32_t c = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i += 4) {
        if (i == skip) continue;
        c ^= get32(_img + i);
        for (int k = 0; k < 32; k++) c = (c & 0x80000000u) ? (c << 1) ^ 0x04C11DB7u : c << 1;
    }
    return c;
}

uint16_t crc16(const uint8_t* d, uint32_t n) {   // CRC-16/CCITT-FALSE — a block's check
    uint16_t c = 0xFFFF;
    while (n--) { c ^= (uint16_t)(*d++) << 8; for (int k = 0; k < 8; k++) c = (c & 0x8000) ? (c << 1) ^ 0x1021 : c << 1; }
    return c;
}

// ── Relay: CAN frames down as export records (BootProtocol.h) ─────────────────
uint8_t  _rec[2 * B::RELAY_EXPORT_WORDS];
uint16_t _recLen;
uint8_t  _out[4 + 2 * (4 + 2 * B::RELAY_EXPORT_WORDS) * 2];   // sync + a few records per call
uint16_t _outLen;

void recFlush() {
    if (!_recLen) return;
    if (_recLen & 1) _rec[_recLen++] = 0x00;   // whole words; 0x00 is padding
    _out[_outLen++] = (uint8_t)B::RELAY_EXPORT_ADDRESS;
    _out[_outLen++] = (uint8_t)(B::RELAY_EXPORT_ADDRESS >> 8);
    _out[_outLen++] = (uint8_t)_recLen;
    _out[_outLen++] = 0;
    memcpy(_out + _outLen, _rec, _recLen);
    _outLen += _recLen;
    _recLen = 0;
    if (_outLen > sizeof(_out) - (4 + sizeof(_rec))) { _io.writeUart(_out, _outLen); _outLen = 0; }
}

void beginFrames() { _outLen = 0; _recLen = 0; for (int i = 0; i < 4; i++) _out[_outLen++] = 0x55; }   // sync

void addFrame(uint16_t id, const uint8_t* d, uint8_t n) {
    uint8_t f[1 + 2 * (2 + 8)];
    if (n > 8) n = 8;
    const uint8_t total = (uint8_t)(2 + n);   // id lo, id hi, data
    uint8_t k = 0;
    f[k++] = B::RELAY_FRAME_MARK | n;
    uint8_t raw[10] = {(uint8_t)id, (uint8_t)(id >> 8)};
    memcpy(raw + 2, d, n);
    for (uint8_t i = 0; i < total; i++) {
        if (raw[i] == 0x55)      { f[k++] = B::RELAY_ESCAPE; f[k++] = B::RELAY_ESC_55; }
        else if (raw[i] == 0x56) { f[k++] = B::RELAY_ESCAPE; f[k++] = B::RELAY_ESC_56; }
        else f[k++] = raw[i];
    }
    if (_recLen + k > sizeof(_rec)) recFlush();   // a record starts on a frame boundary
    memcpy(_rec + _recLen, f, k);
    _recLen += k;
}

void endFrames() { recFlush(); if (_outLen) _io.writeUart(_out, _outLen); _outLen = 0; }

void sendCmd(const uint8_t* d, uint8_t n) { beginFrames(); addFrame(_cmdId, d, n); endFrames(); }

void sendBlock(uint16_t b) {
    const uint8_t* chunk = _img + (uint32_t)b * B::BLOCK_SIZE;   // past _len the buffer is 0xFF
    uint8_t h[5] = {(uint8_t)B::Cmd::BLOCK, (uint8_t)b, (uint8_t)(b >> 8)};
    put16(h + 3, crc16(chunk, B::BLOCK_SIZE));
    beginFrames();
    addFrame(_cmdId, h, sizeof h);
    for (uint8_t k = 0; k < B::DATA_FRAMES; k++) {
        uint8_t d[8] = {(uint8_t)((uint8_t)B::Cmd::DATA_0 + k)};
        const uint8_t at = k * B::DATA_BYTES, cnt = at + B::DATA_BYTES <= B::BLOCK_SIZE ? B::DATA_BYTES : B::BLOCK_SIZE - at;
        memcpy(d + 1, chunk + at, cnt);
        addFrame(_cmdId, d, 1 + cnt);
    }
    endFrames();
}

// ── Reporting ─────────────────────────────────────────────────────────────────
void sendStatus(uint32_t now) {
    _lastStatusMs = now;
    if (!clientHere(now)) return;
    uint8_t p[8] = {};
    p[UPL_STATUS_PHASE]   = _phase;
    p[UPL_STATUS_TARGET]  = _unconfigured ? UPL_TARGET_UNCONFIGURED : _target;
    p[UPL_STATUS_ATTEMPT] = _attempt;
    put16(p + UPL_STATUS_BLOCK, _acked);
    put16(p + UPL_STATUS_BLOCKS, _blocks);
    _io.send(CAL_T_UPL_STATUS, 0, p, sizeof p);
}

void enter(UplPhase ph, uint32_t now) { _phase = ph; _phaseMs = now; _sentOnce = false; sendStatus(now); }

void finish(UplResult r, uint32_t now) {
    // An unconfigured board that took its image is now that image's node — name it.
    _lastTarget = !_unconfigured ? _target : r == UPL_RESULT_OK ? _id->nodeId : UPL_TARGET_UNCONFIGURED;
    _lastResult = r;
    const Seen& s = _seen[_id->nodeId & 63];
    if (r == UPL_RESULT_OK || r == UPL_RESULT_VERSION_MISMATCH) memcpy(_lastFw, s.fw, 3);
    else memset(_lastFw, 0, 3);
    _state = UPL_STATE_STAGED;   // the image stays staged: the client may flash it again
    if (clientHere(now)) {
        uint8_t p[8] = {};
        p[UPL_RESULT_RESULT]   = r;
        p[UPL_RESULT_TARGET]   = _lastTarget;
        memcpy(p + UPL_RESULT_MAJOR, _lastFw, 3);
        p[UPL_RESULT_ATTEMPTS] = (uint8_t)(_attempt + 1);
        put16(p + UPL_RESULT_SECONDS, (uint16_t)((now - _flashMs) / 1000));
        _io.send(CAL_T_UPL_RESULT, 0, p, sizeof p);
    }
}

void address(uint8_t nodeId) {
    _target = nodeId;
    _cmdId  = B::canIdBootCmd(nodeId);
    _rspId  = B::canIdBootRsp(nodeId);
}

// A transport failure: try the whole flash again, from entering the bootloader.
void retry(UplResult ifExhausted, uint32_t now) {
    if (_attempt >= UPL_RETRIES) { finish(ifExhausted, now); return; }
    _attempt++;
    enter(UPL_PHASE_ENTERING_BOOT, now);
}

// ── Bootloader replies ────────────────────────────────────────────────────────
void onBootReply(uint16_t id, const uint8_t* d, uint8_t n, uint32_t now) {
    if (_state != UPL_STATE_FLASHING || !n) return;
    if (_unconfigured && id == B::UNCONFIGURED_RSP_ID && !_uidKnown && d[0] == (uint8_t)B::Reply::ANNOUNCE && n >= B::ANNOUNCE_LEN) {
        _uid = d[(uint8_t)B::AnnounceField::UID16] | (d[(uint8_t)B::AnnounceField::UID16 + 1] << 8);
        _uidKnown = true;
        return;
    }
    if (id != _rspId) return;
    const B::Reply code = (B::Reply)d[0];

    switch (_phase) {
    case UPL_PHASE_ENTERING_BOOT: {
        if (code != B::Reply::CONNECTED || n < B::ANNOUNCE_LEN) return;
        enter(UPL_PHASE_CONNECTED, now);
        const uint8_t kind = d[(uint8_t)B::AnnounceField::KIND];
        const uint32_t room = (uint32_t)d[(uint8_t)B::AnnounceField::FLASH_KB] * 1024u - B::BOOTLOADER_SIZE;
        if (kind != 0xFF && kind != _id->nodeKind) { finish(UPL_RESULT_KIND_MISMATCH, now); return; }
        if (_len > room) { finish(UPL_RESULT_TOO_BIG_FOR_BOARD, now); return; }
        if (!_unconfigured && _id->nodeId != _target) {
            if (_newId != _id->nodeId) { finish(UPL_RESULT_ID_MISMATCH, now); return; }
            enter(UPL_PHASE_SETTING_ID, now);
            return;
        }
        enter(UPL_PHASE_ERASING, now);
        return;
    }
    case UPL_PHASE_SETTING_ID:
        if (code != B::Reply::NODE_SET || n < 2) return;
        if (d[1] != (uint8_t)B::Status::OK) { finish(UPL_RESULT_ID_MISMATCH, now); return; }
        address(_newId);   // the board resets and answers on its new BOOT_RSP_n, app invalid
        enter(UPL_PHASE_ENTERING_BOOT, now);
        return;
    case UPL_PHASE_ERASING:
        if (code != B::Reply::BEGUN || n < 2) return;
        if (d[1] != (uint8_t)B::Status::OK) { retry(UPL_RESULT_WRITE_FAILED, now); return; }
        _acked = _sent = 0;
        _goback = 0xFFFF;
        _stalls = 0;
        _ackDeadline = now + ACK_TIMEOUT_MS;
        enter(UPL_PHASE_WRITING, now);
        return;
    case UPL_PHASE_WRITING: {
        if (code != B::Reply::BLOCK_ACK || n < 6) return;
        const uint8_t st = d[3];
        const uint16_t next = d[4] | (d[5] << 8);
        // The board's `next` never goes back within a session: if it does, the board reset and
        // lost it (its bootloader answers every block OUT_OF_ORDER, next 0) — start over.
        if (next < _acked) { retry(UPL_RESULT_WRITE_FAILED, now); return; }
        _stalls = 0;
        _ackDeadline = now + ACK_TIMEOUT_MS;
        if (st == (uint8_t)B::Status::OK) { if (next > _acked) _acked = next; }
        else if (st == (uint8_t)B::Status::OUT_OF_ORDER || st == (uint8_t)B::Status::BAD_CRC) {
            if (next != _goback) { _goback = next; _sent = next; }   // one go-back per loss
            if (next > _acked) _acked = next;
        } else { retry(UPL_RESULT_WRITE_FAILED, now); return; }
        if (_acked >= _blocks) enter(UPL_PHASE_VERIFYING, now);
        return;
    }
    case UPL_PHASE_VERIFYING: {
        if (code != B::Reply::ENDED || n < 2) return;
        const B::Status st = (B::Status)d[1];
        if (st == B::Status::OK)                { enter(UPL_PHASE_REBOOTING, now); return; }
        if (st == B::Status::WRONG_NODE)        { finish(UPL_RESULT_ID_MISMATCH, now); return; }
        if (st == B::Status::WRONG_KIND)        { finish(UPL_RESULT_KIND_MISMATCH, now); return; }
        retry(UPL_RESULT_VERIFY_FAILED, now);
        return;
    }
    case UPL_PHASE_REBOOTING:
        if (code != B::Reply::GOING || n < 2) return;
        if (d[1] != (uint8_t)B::Status::OK) { retry(UPL_RESULT_VERIFY_FAILED, now); return; }
        _confirmFromMs = now;
        enter(UPL_PHASE_CONFIRMING, now);
        return;
    default:
        return;
    }
}

int hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
bool hexBytes(const char* s, uint8_t* out, uint16_t n) {
    for (uint16_t i = 0; i < n; i++) {
        const int hi = hexNibble(s[2 * i]), lo = hexNibble(s[2 * i + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = (uint8_t)(hi << 4 | lo);
    }
    return true;
}

bool startsWith(const char* line, uint16_t len, const char* prefix, uint16_t* at) {
    const uint16_t n = (uint16_t)strlen(prefix);
    if (len <= n || memcmp(line, prefix, n) != 0 || line[n] != ' ') return false;
    *at = n + 1;
    return true;
}

}  // namespace

namespace Uploader {

void begin(const Io& io) { _io = io; memset(_img, 0xFF, sizeof _img); }

bool handles(uint8_t type) {
    return type == CAL_T_UPL_INFO || type == CAL_T_UPL_BEGIN || type == CAL_T_UPL_DATA
        || type == CAL_T_UPL_END || type == CAL_T_UPL_FLASH || type == CAL_T_UPL_ABORT;
}

bool busy()     { return _state != UPL_STATE_IDLE; }
bool ownsUart() { return _state == UPL_STATE_FLASHING; }

void onClientFrame(uint32_t now) { _lastClientMs = now; }

void onFrame(uint8_t type, uint8_t seq, const uint8_t* p, uint16_t len, uint32_t now) {
    (void)len;
    _lastClientMs = now;
    switch (type) {
    case CAL_T_UPL_INFO: {
        uint8_t a[16] = {};
        a[UPL_INFO_ACK_PROTO]        = UPL_PROTO_VERSION;
        a[UPL_INFO_ACK_MAX_IMAGE_KB] = (uint8_t)(UPL_MAX_IMAGE_BYTES / 1024);
        put16(a + UPL_INFO_ACK_CHUNK, UPL_CHUNK_BYTES);
        a[UPL_INFO_ACK_STATE]        = _state;
        a[UPL_INFO_ACK_LAST_TARGET]  = _lastTarget;
        a[UPL_INFO_ACK_LAST_RESULT]  = _lastResult;
        memcpy(a + UPL_INFO_ACK_LAST_MAJOR, _lastFw, 3);
        _io.send(CAL_T_UPL_INFO_ACK, seq, a, sizeof a);   // bundled bridge version: 0.0.0 until #367
        return;
    }
    case CAL_T_UPL_BEGIN: {
        if (_state == UPL_STATE_FLASHING) { _io.nack(type, seq, CAL_NACK_BUSY, 0xFF); return; }
        const uint32_t n = get32(p + UPL_BEGIN_LENGTH);
        if (!n || n > UPL_MAX_IMAGE_BYTES || (n & 3)) { _io.nack(type, seq, CAL_NACK_TOO_LARGE, 0xFF); return; }
        memset(_img, 0xFF, sizeof _img);
        _len   = n;
        _crc   = get32(p + UPL_BEGIN_CRC);
        _state = UPL_STATE_STAGING;
        _io.ack(type, seq);
        return;
    }
    case CAL_T_UPL_DATA: {
        if (_state != UPL_STATE_STAGING) { _io.nack(type, seq, CAL_NACK_NO_SESSION, 0xFF); return; }
        const uint32_t off = get32(p + UPL_DATA_OFFSET);
        if (off % UPL_CHUNK_BYTES || off >= _len) { _io.nack(type, seq, CAL_NACK_BAD_OFFSET, 0xFF); return; }
        const uint32_t cnt = _len - off < UPL_CHUNK_BYTES ? _len - off : UPL_CHUNK_BYTES;
        memcpy(_img + off, p + UPL_DATA_BYTES, cnt);   // past _len stays 0xFF: the blocks' padding
        _io.ack(type, seq);
        return;
    }
    case CAL_T_UPL_END: {
        if (_state != UPL_STATE_STAGING) { _io.nack(type, seq, CAL_NACK_NO_SESSION, 0xFF); return; }
        if (imageCrc(_len) != _crc) { _io.nack(type, seq, CAL_NACK_IMAGE_CRC, 0xFF); return; }
        if (_id->magic != FirmwareImage::MAGIC || _id->imageLength != _len || _id->imageCrc != _crc) {
            _io.nack(type, seq, CAL_NACK_BAD_IMAGE, 0xFF);
            return;
        }
        _state = UPL_STATE_STAGED;
        _io.ack(type, seq);
        return;
    }
    case CAL_T_UPL_FLASH: {
        if (_state == UPL_STATE_IDLE || _state == UPL_STATE_FLASHING) {
            _io.nack(type, seq, _state == UPL_STATE_IDLE ? CAL_NACK_NO_SESSION : CAL_NACK_BUSY, 0xFF);
            return;
        }
        if (_state != UPL_STATE_STAGED) { _io.nack(type, seq, CAL_NACK_NOT_STAGED, 0xFF); return; }
        const uint8_t t = p[UPL_FLASH_TARGET];
        if (t != UPL_TARGET_UNCONFIGURED && (t > 63)) { _io.nack(type, seq, CAL_NACK_BAD_INDEX, t); return; }
        _io.ack(type, seq);
        _state        = UPL_STATE_FLASHING;
        _flashMs      = now;
        _attempt      = 0;
        _newId        = p[UPL_FLASH_NEW_NODE_ID];
        _unconfigured = t == UPL_TARGET_UNCONFIGURED;
        _uidKnown     = false;
        _blocks       = (uint16_t)((_len + B::BLOCK_SIZE - 1) / B::BLOCK_SIZE);
        _acked        = 0;
        if (t == UPL_TARGET_BRIDGE) { _target = 0; finish(UPL_RESULT_UNSUPPORTED, now); return; }   // #367
        if (_unconfigured) { _target = 0xFF; _cmdId = B::UNCONFIGURED_CMD_ID; _rspId = B::UNCONFIGURED_RSP_ID; }
        else address(t);
        enter(UPL_PHASE_ENTERING_BOOT, now);
        return;
    }
    case CAL_T_UPL_ABORT:
        if (_state == UPL_STATE_FLASHING) { _io.nack(type, seq, CAL_NACK_BUSY, 0xFF); return; }
        _state = UPL_STATE_IDLE;
        _io.ack(type, seq);
        return;
    default:
        return;
    }
}

bool onBridgeLine(const char* line, uint16_t len, uint32_t now) {
    uint16_t at;
    if (startsWith(line, len, BOOT_RSP_MSG_NAME, &at)) {
        // <id(3)><len(1)><data>
        if (len >= at + 4) {
            const int a = hexNibble(line[at]), b = hexNibble(line[at + 1]), c = hexNibble(line[at + 2]), n = hexNibble(line[at + 3]);
            uint8_t d[8];
            if (a >= 0 && b >= 0 && c >= 0 && n >= 0 && n <= 8 && len >= at + 4 + 2 * n && hexBytes(line + at + 4, d, (uint16_t)n))
                onBootReply((uint16_t)(a << 8 | b << 4 | c), d, (uint8_t)n, now);
        }
        return ownsUart();
    }
    if (startsWith(line, len, "_NODE_STATUS", &at) && len >= at + 38) {
        uint8_t r[19];
        if (hexBytes(line + at, r, sizeof r) && r[0] < 64) {
            Seen& s = _seen[r[0]];
            s.present = r[1] != 0;
            memcpy(s.fw, r + 14, 3);
            s.bootloader = r[17];
            s.bootState  = r[18];
            s.ms = now;
        }
    }
    return false;
}

void tick(uint32_t now) {
    if (_state == UPL_STATE_STAGING || _state == UPL_STATE_STAGED) {
        if (now - _lastClientMs > SESSION_IDLE_MS) _state = UPL_STATE_IDLE;
        return;
    }
    if (_state != UPL_STATE_FLASHING) return;
    if (now - _lastStatusMs >= STATUS_EVERY_MS && _phase == UPL_PHASE_WRITING) sendStatus(now);

    switch (_phase) {
    case UPL_PHASE_ENTERING_BOOT:
        if (now - _phaseMs > ENTER_TIMEOUT_MS) {
            const Seen& s = _seen[_target & 63];
            if (_attempt > 0) { retry(UPL_RESULT_WRITE_FAILED, now); return; }   // was mid-flash: try again
            finish(!_unconfigured && s.present && s.bootloader == 0 ? UPL_RESULT_NO_BOOTLOADER : UPL_RESULT_NO_RESPONSE, now);
            return;
        }
        if (_unconfigured && !_uidKnown) return;   // wait for its announcement
        if (now - _lastSendMs >= CONNECT_EVERY_MS) {
            _lastSendMs = now;
            uint8_t c[3] = {(uint8_t)B::Cmd::CONNECT, (uint8_t)_uid, (uint8_t)(_uid >> 8)};
            sendCmd(c, _unconfigured ? 3 : 1);
        }
        return;
    case UPL_PHASE_SETTING_ID:
        if (!_sentOnce) { _sentOnce = true; uint8_t c[2] = {(uint8_t)B::Cmd::SET_NODE_ID, _newId}; sendCmd(c, 2); }
        if (now - _phaseMs > REPLY_TIMEOUT_MS) retry(UPL_RESULT_ID_MISMATCH, now);
        return;
    case UPL_PHASE_ERASING:
        if (!_sentOnce) {
            _sentOnce = true;
            uint8_t c[5] = {(uint8_t)B::Cmd::BEGIN, (uint8_t)_len, (uint8_t)(_len >> 8), (uint8_t)(_len >> 16), (uint8_t)(_len >> 24)};
            sendCmd(c, 5);
        }
        if (now - _phaseMs > ERASE_TIMEOUT_MS) retry(UPL_RESULT_WRITE_FAILED, now);
        return;
    case UPL_PHASE_WRITING:
        if ((int32_t)(now - _ackDeadline) >= 0) {   // no ack for ACK_TIMEOUT_MS: resend from the first unacked block
            if (++_stalls > MAX_STALLS) { retry(UPL_RESULT_WRITE_FAILED, now); return; }
            _sent = _acked;
            _ackDeadline = now + ACK_TIMEOUT_MS;
        }
        // One block per tick, so loop() keeps serving the joystick and USB between them.
        if (_sent < _blocks && _sent - _acked < B::WINDOW_BLOCKS) sendBlock(_sent++);
        return;
    case UPL_PHASE_VERIFYING:
        if (!_sentOnce) {
            _sentOnce = true;
            uint8_t c[5] = {(uint8_t)B::Cmd::END, (uint8_t)_crc, (uint8_t)(_crc >> 8), (uint8_t)(_crc >> 16), (uint8_t)(_crc >> 24)};
            sendCmd(c, 5);
        }
        if (now - _phaseMs > REPLY_TIMEOUT_MS) retry(UPL_RESULT_VERIFY_FAILED, now);
        return;
    case UPL_PHASE_REBOOTING:
        if (!_sentOnce) { _sentOnce = true; uint8_t c[1] = {(uint8_t)B::Cmd::GO}; sendCmd(c, 1); }
        if (now - _phaseMs > REPLY_TIMEOUT_MS) { _confirmFromMs = now; enter(UPL_PHASE_CONFIRMING, now); }   // GOING lost: it reboots anyway
        return;
    case UPL_PHASE_CONFIRMING: {
        // The app reports itself through PanelBridge: a row for the image's NODE_ID, app running,
        // newer than GO.
        const Seen& s = _seen[_id->nodeId & 63];
        const bool fresh = s.ms && (int32_t)(s.ms - _confirmFromMs) >= 0;
        if (fresh && s.present && s.bootState == NODE_BOOT_STATE_APP) {
            const bool same = s.fw[0] == _id->fwMajor && s.fw[1] == _id->fwMinor && s.fw[2] == _id->fwPatch;
            finish(same ? UPL_RESULT_OK : UPL_RESULT_VERSION_MISMATCH, now);
            return;
        }
        if (now - _phaseMs > CONFIRM_TIMEOUT_MS) finish(UPL_RESULT_TIMEOUT, now);
        return;
    }
    default:
        return;
    }
}

}  // namespace Uploader
