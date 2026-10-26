#pragma once
/**
 * @file BootProtocol.h
 * @brief The CAN bootloader's wire contract (#363) — frame IDs, commands, replies, status codes,
 *        the entry flag and the protocol version.
 *
 * One source for every side of an update: the bootloader (Firmware/Bootloader), the running
 * firmware that sends a node into it (CANProtocol, #365), the Gateway uploader (#366) and the
 * client sync. Header-only, no Arduino, no HAL — the bootloader builds bare-metal.
 *
 * Every wire value is a named `enum class X : T { NAME = literal }` or `constexpr T NAME = literal`
 * so the client sync can read it (#346). Full protocol: FirmwarePlan/12-bootloader.md.
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */
#include <stdint.h>

namespace OpenSkyhawk {
namespace Boot {

/// Bumped on any change to the frames below. A board reports it in READY_n byte 3 (#365); 0 = no bootloader.
constexpr uint8_t PROTOCOL_VERSION = 1;

// ── CAN frame IDs (11-bit) ────────────────────────────────────────────────────
// The lowest priority on the bus, so update traffic never delays a control.

constexpr uint16_t CMD_ID_BASE = 0x740;  ///< BOOT_CMD_n = 0x740 + n — host (bridge) → node n's bootloader
constexpr uint16_t RSP_ID_BASE = 0x780;  ///< BOOT_RSP_n = 0x780 + n — node n's bootloader → host

/** @brief BOOT_CMD_n for node n (0x740-0x77F). */
constexpr uint16_t canIdBootCmd(uint8_t n) { return CMD_ID_BASE + n; }
/** @brief BOOT_RSP_n for node n (0x780-0x7BF). */
constexpr uint16_t canIdBootRsp(uint8_t n) { return RSP_ID_BASE + n; }

/// A bootloader with no NODE_ID yet (option byte Data0 = 0xFF, no valid app) announces here.
constexpr uint16_t UNCONFIGURED_RSP_ID = 0x7C1;
/// Commands to an unconfigured bootloader, addressed by chip UID — reserved for #364, unused in #363.
constexpr uint16_t UNCONFIGURED_CMD_ID = 0x7C0;

// ── Host → bootloader (BOOT_CMD_n), byte 0 ───────────────────────────────────

enum class Cmd : uint8_t {
    CONNECT = 0x01,  ///< open a session → Reply::CONNECTED
    BEGIN   = 0x02,  ///< [len:u32] image length; checks capacity, erases the app's first page → Reply::BEGUN
    BLOCK   = 0x10,  ///< [index:u16, crc16:u16] header of a 64-byte block; DATA_0..DATA_9 follow
    DATA_0  = 0x20,  ///< 7 data bytes of the current block; DATA_k = DATA_0 + k carries bytes 7k..7k+6
    DATA_9  = 0x29,  ///< the last DATA frame of a block (bytes 63..69; only byte 63 is used)
    END     = 0x30,  ///< [crc:u32] image CRC; verifies, then commits block 0 → Reply::ENDED
    GO      = 0x40,  ///< leave the bootloader: reset into the app → Reply::GOING
};

// ── Bootloader → host (BOOT_RSP_n), byte 0 ───────────────────────────────────

enum class Reply : uint8_t {
    CONNECTED   = 0x81,  ///< [AnnounceField bytes 1-5] — same layout as ANNOUNCE
    BEGUN       = 0x82,  ///< [status]
    BLOCK_ACK   = 0x90,  ///< [index:u16, status] after DATA_9
    ANNOUNCE    = 0xA0,  ///< every ANNOUNCE_INTERVAL_MS while no session: [AnnounceField bytes 1-5]
    ENDED       = 0xB0,  ///< [status, crc:u32 computed]
    GOING       = 0xC0,  ///< [status]; on OK the board resets into the app
};

/// Byte offsets in ANNOUNCE / CONNECTED.
enum class AnnounceField : uint8_t {
    CODE     = 0,  ///< Reply::ANNOUNCE or Reply::CONNECTED
    PROTOCOL = 1,  ///< PROTOCOL_VERSION
    NODE_ID  = 2,  ///< option byte Data0 (0xFF = unconfigured)
    KIND     = 3,  ///< option byte Data1 — NodeKind (0xFF = unconfigured)
    STATE    = 4,  ///< BootState
    FLASH_KB = 5,  ///< the chip's flash size in KB (64 / 128) — capacity = FLASH_KB × 1024 − BOOTLOADER_SIZE
};
constexpr uint8_t ANNOUNCE_LEN = 6;

/// Why the board is in its bootloader (AnnounceField::STATE).
enum class BootState : uint8_t {
    REQUESTED    = 0x01,  ///< the app asked (entry flag) — the app may still be valid
    APP_INVALID  = 0x02,  ///< no app, or its identity block / CRC check failed
    UNCONFIGURED = 0x03,  ///< no NODE_ID yet (Data0 = 0xFF) and no valid app
    // 0x04-0xFF reserved (#364: app unstable, caught at boot)
};

/// Result byte of BEGUN / BLOCK_ACK / ENDED / GOING.
enum class Status : uint8_t {
    OK           = 0x00,
    BAD_CRC      = 0x01,  ///< block CRC-16 or image CRC-32 mismatch
    FLASH_ERROR  = 0x02,  ///< erase / program / verify failed
    OUT_OF_RANGE = 0x03,  ///< image or block past the end of flash, or zero length
    OUT_OF_ORDER = 0x04,  ///< no BEGIN, a block skipped, or END before the last block
    NO_APP       = 0x05,  ///< GO without a valid app
    BAD_IMAGE    = 0x06,  ///< no identity block, or its length / CRC disagree with the transfer
};

// ── Transfer shape ────────────────────────────────────────────────────────────

constexpr uint16_t BLOCK_SIZE        = 64;  ///< bytes per block; the image is padded with 0xFF to a whole block
constexpr uint8_t  DATA_BYTES        = 7;   ///< payload bytes per DATA frame (byte 0 = Cmd::DATA_k)
constexpr uint8_t  DATA_FRAMES       = 10;  ///< DATA frames per block (70 bytes carried, 64 used)
constexpr uint16_t ANNOUNCE_INTERVAL_MS = 1000;
// Block CRC: CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) over the 64 bytes — the same CRC the
// Gateway's CDC frames use. Image CRC: the identity block's CRC-32/MPEG-2 (FirmwareImage.h).

// ── Layout and entry ──────────────────────────────────────────────────────────

constexpr uint32_t BOOTLOADER_SIZE = 0x1000;  ///< 4 KB = one F103 write-protect block; apps start here
constexpr uint32_t APP_OFFSET      = BOOTLOADER_SIZE;

/// The app sets backup register DR1 to ENTRY_MAGIC and resets to enter the bootloader (#365). The
/// bootloader clears it on read. The backup domain survives a reset, not a power cycle.
constexpr uint8_t  ENTRY_BKP_REGISTER = 1;
constexpr uint16_t ENTRY_MAGIC        = 0xB007;

}  // namespace Boot
}  // namespace OpenSkyhawk
