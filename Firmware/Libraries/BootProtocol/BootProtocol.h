#pragma once
/**
 * @file BootProtocol.h
 * @brief The CAN bootloader's wire contract (#363) — frame IDs, commands, replies, status codes,
 *        the entry flag and the protocol version.
 *
 * One source for every side of an update: the bootloader (Firmware/Bootloader), the running
 * firmware that sends a node into it (STM32Board / CANProtocol, #365), the bridge relay (PanelBridge,
 * #365), the Gateway uploader (#366) and the client sync. Header-only, no Arduino, no HAL — the bootloader builds bare-metal.
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
// Below every control and status frame, so update traffic never delays a control; only CTRL_BCAST
// (0x7E0) is lower (#389).

constexpr uint16_t CMD_ID_BASE = 0x740;  ///< BOOT_CMD_n = 0x740 + n — host (bridge) → node n's bootloader
constexpr uint16_t RSP_ID_BASE = 0x780;  ///< BOOT_RSP_n = 0x780 + n — node n's bootloader → host

/** @brief BOOT_CMD_n for node n (0x740-0x77F). */
constexpr uint16_t canIdBootCmd(uint8_t n) { return CMD_ID_BASE + n; }
/** @brief BOOT_RSP_n for node n (0x780-0x7BF). */
constexpr uint16_t canIdBootRsp(uint8_t n) { return RSP_ID_BASE + n; }

/// A bootloader with no NODE_ID yet (option byte Data0 = 0xFF, no valid app) announces here.
constexpr uint16_t UNCONFIGURED_RSP_ID = 0x7C1;
/// Commands to an unconfigured bootloader (#364). CONNECT names the board's UID hash
/// (AnnounceField::UID16); only that board binds the session and answers on UNCONFIGURED_RSP_ID.
/// One unconfigured board on the bus at a time.
constexpr uint16_t UNCONFIGURED_CMD_ID = 0x7C0;

// ── Host → bootloader (BOOT_CMD_n), byte 0 ───────────────────────────────────

enum class Cmd : uint8_t {
    CONNECT = 0x01,  ///< open a session → Reply::CONNECTED. On UNCONFIGURED_CMD_ID: [uid16:u16]
    BEGIN   = 0x02,  ///< [len:u32] image length; checks capacity, erases every page the image needs (page 0 first) → Reply::BEGUN
    BLOCK   = 0x10,  ///< [index:u16, crc16:u16] header of a 64-byte block; DATA_0..DATA_9 follow
    DATA_0  = 0x20,  ///< 7 data bytes of the current block; DATA_k = DATA_0 + k carries bytes 7k..7k+6
    DATA_9  = 0x29,  ///< the last DATA frame of a block (bytes 63..69; only byte 63 is used)
    END     = 0x30,  ///< [crc:u32] image CRC; verifies, then commits block 0 → Reply::ENDED
    GO      = 0x40,  ///< leave the bootloader: reset into the app → Reply::GOING
    SET_NODE_ID = 0x50,  ///< [id] after CONNECT: invalidate the app, set NODE_ID (1-63), reset → Reply::NODE_SET (#364)
};

// ── Bootloader → host (BOOT_RSP_n), byte 0 ───────────────────────────────────

enum class Reply : uint8_t {
    CONNECTED   = 0x81,  ///< [AnnounceField bytes 1-5] — same layout as ANNOUNCE
    BEGUN       = 0x82,  ///< [status]
    BLOCK_ACK   = 0x90,  ///< [index:u16, status, next:u16] after DATA_9 — next = the first block not yet
                         ///< written, where a host resumes after a loss (#366)
    ANNOUNCE    = 0xA0,  ///< every ANNOUNCE_INTERVAL_MS while no image is arriving, and during a transfer
                         ///< idle that long: [AnnounceField bytes 1-5]
    ENDED       = 0xB0,  ///< [status, crc:u32 computed]
    GOING       = 0xC0,  ///< [status]; on OK the board resets into the app
    NODE_SET    = 0xD0,  ///< [status]; on OK the board resets and answers on its new BOOT_RSP_n
};

/// Byte offsets in ANNOUNCE / CONNECTED.
enum class AnnounceField : uint8_t {
    CODE     = 0,  ///< Reply::ANNOUNCE or Reply::CONNECTED
    PROTOCOL = 1,  ///< PROTOCOL_VERSION
    ID       = 2,  ///< NODE_ID — option byte Data0 (0xFF = unconfigured). Not "NODE_ID": every node build defines that macro
    KIND     = 3,  ///< option byte Data1 — NodeKind (0xFF = unconfigured)
    STATE    = 4,  ///< BootState
    FLASH_KB = 5,  ///< the chip's flash size in KB (64 / 128) — capacity = FLASH_KB × 1024 − BOOTLOADER_SIZE
    UID16    = 6,  ///< u16: low half of CRC-32/MPEG-2 over the chip's 96-bit UID — tells boards apart (#364)
};
constexpr uint8_t  ANNOUNCE_LEN = 8;
constexpr uint32_t UID_ADDRESS  = 0x1FFFF7E8;  ///< the F103's 96-bit unique ID

/// Why the board is in its bootloader (AnnounceField::STATE).
enum class BootState : uint8_t {
    REQUESTED    = 0x01,  ///< the app asked (entry flag) — the app may still be valid
    APP_INVALID  = 0x02,  ///< no app, or its identity block / CRC check failed
    UNCONFIGURED = 0x03,  ///< no NODE_ID yet (Data0 = 0xFF) and no valid app
    APP_UNSTABLE = 0x04,  ///< the app reset ATTEMPT_LIMIT times without reporting healthy (#364)
    CAUGHT       = 0x05,  ///< a CONNECT arrived in the CATCH_WINDOW_MS after reset (#364) — the app may be valid
    // 0x06-0xFF reserved
};

/// Result byte of BEGUN / BLOCK_ACK / ENDED / GOING.
enum class Status : uint8_t {
    OK           = 0x00,
    BAD_CRC      = 0x01,  ///< block CRC-16 or image CRC-32 mismatch
    FLASH_ERROR  = 0x02,  ///< erase / program / verify failed
    OUT_OF_RANGE = 0x03,  ///< image past the end of flash, zero length, or a SET_NODE_ID id outside 1-63 / on the bridge
    OUT_OF_ORDER = 0x04,  ///< no BEGIN, a block skipped, or END before the last block
    NO_APP       = 0x05,  ///< GO without a valid app
    BAD_IMAGE    = 0x06,  ///< no identity block, or its length / CRC disagree with the transfer
    WRONG_NODE   = 0x07,  ///< the image's nodeId isn't this board's NODE_ID — SET_NODE_ID first (#364)
    WRONG_KIND   = 0x08,  ///< the image's nodeKind isn't this board's kind — never accepted (#364)
};

// ── Transfer shape ────────────────────────────────────────────────────────────

constexpr uint16_t BLOCK_SIZE        = 64;  ///< bytes per block; the image is padded with 0xFF to a whole block
/// Blocks a host may send ahead of the last ack (#366). The bootloader buffers their frames while it
/// writes flash; a lost block is resent from BLOCK_ACK's `next` (go-back-N). BEGIN erases every page
/// the image needs before it replies, so no erase stalls the stream.
constexpr uint8_t  WINDOW_BLOCKS     = 4;
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

// ── Bootloader info block (#365) ──────────────────────────────────────────────

/// The last 16 bytes of the bootloader's 4 KB. A running app reads it to know a bootloader is present
/// (and so whether it may honour an enter request) and which protocol it speaks (READY_n byte 3).
constexpr uint32_t INFO_ADDRESS = 0x08000FF0;
constexpr uint32_t INFO_MAGIC   = 0x4C42534Fu;   ///< "OSBL" in memory order

/// BootloaderInfo `transports` bits (#367): how the installed bootloader can be reached. An app only
/// resets into it on a request that arrived the same way, so a board can't strand itself in a bootloader
/// nothing can talk to. (0xFF from a #365 / #366 bootloader reads as both.)
constexpr uint8_t TRANSPORT_CAN    = 0x01;   ///< BOOT_CMD_n / BOOT_RSP_n — the node build
constexpr uint8_t TRANSPORT_SERIAL = 0x02;   ///< USART2 to the Gateway's RP2040 — the bridge build

struct BootloaderInfo {
    uint32_t magic;            ///< INFO_MAGIC
    uint8_t  protocolVersion;  ///< PROTOCOL_VERSION of the bootloader that is installed
    uint8_t  transports;       ///< TRANSPORT_* bits
    uint8_t  reserved[10];     ///< 0xFF
};
static_assert(sizeof(BootloaderInfo) == 16, "the bootloader info block is 16 bytes");

// ── Relay through the Gateway (#365) ──────────────────────────────────────────
// The Gateway → PanelBridge UART carries only the DCS-BIOS export stream, so a host sends boot
// commands the way it asks for the roster: by writing reserved export addresses. Each export record at
// RELAY_EXPORT_ADDRESS (up to RELAY_EXPORT_WORDS words, low byte of each word first) carries a byte
// stream of whole CAN frames (#366):
//   RELAY_FRAME_MARK | len   (len 0-8), id lo, id hi, len data bytes
// with every 0x55 and 0x56 inside a frame sent as RELAY_ESCAPE + RELAY_ESC_55 / RELAY_ESC_56 — the
// DCS-BIOS parser resyncs on any four 0x55 bytes in a row, which image data could contain. Bytes
// between frames (padding) are 0x00 and ignored; every record starts on a frame boundary. PanelBridge
// puts each frame on CAN — only BOOT_CMD_n for n = 1-63 and UNCONFIGURED_CMD_ID — and never
// broadcasts the window as CTRL_BCAST. Every BOOT_RSP_n / UNCONFIGURED_RSP_ID frame goes the other way
// as one DCS-BIOS message: `_BOOT_RSP <id(3 hex)><length(1 hex)><data(2 hex per byte)>`.
constexpr uint16_t RELAY_EXPORT_ADDRESS = 0xFF10;
constexpr uint8_t  RELAY_EXPORT_WORDS   = 32;     ///< 0xFF10-0xFF4F: 64 bytes per record
constexpr uint8_t  RELAY_FRAME_MARK     = 0xE0;   ///< | len: a frame starts
constexpr uint8_t  RELAY_ESCAPE         = 0x56;
constexpr uint8_t  RELAY_ESC_55         = 0x01;   ///< after RELAY_ESCAPE: decodes to 0x55
constexpr uint8_t  RELAY_ESC_56         = 0x02;   ///< after RELAY_ESCAPE: decodes to 0x56
#define BOOT_RSP_MSG_NAME "_BOOT_RSP"

// ── The bridge's serial transport (#367) ──────────────────────────────────────
// The Gateway's PanelBridge is the CAN master: nothing on the bus can update it. Its bootloader is the
// `bridge` build of the same source (Firmware/Bootloader), which takes the same commands and answers
// with the same replies, as frames on USART2 (PA2 / PA3, the link to the Gateway's RP2040) instead of
// CAN — both transports don't fit 4 KB. The id is the CAN id the frame would have had (BOOT_CMD_0 /
// BOOT_RSP_0). One frame on the wire:
//   SERIAL_FRAME_START, id lo, id hi, len (0-8), data, sum   (sum = id lo + id hi + len + data, mod 256)
constexpr uint8_t  SERIAL_FRAME_START = 0xAA;
constexpr uint32_t SERIAL_BAUD        = 250000;   ///< the Gateway link's rate (FirmwarePlan/03)
/// A host writes this export address (any value) to ask the bridge's app to reset into its bootloader
/// — the bridge's counterpart of a node's CONNECT on BOOT_CMD_n. Honoured only with a bootloader installed.
constexpr uint16_t BRIDGE_ENTER_EXPORT_ADDRESS = 0xFF02;

/// Crash-loop counter (#364): the bootloader adds one before every jump to the app; the app sets it
/// back to 0 once it is healthy (STM32Board::markBootHealthy()). At ATTEMPT_LIMIT the bootloader stays
/// (BootState::APP_UNSTABLE). Cleared by a committed image and by GO; lost on a power cycle.
constexpr uint8_t ATTEMPTS_BKP_REGISTER = 2;
constexpr uint8_t ATTEMPT_LIMIT         = 3;

constexpr uint16_t CATCH_WINDOW_MS  = 200;    ///< after every reset, a CONNECT on BOOT_CMD_n keeps the board here (#364)
constexpr uint16_t IDLE_TIMEOUT_MS  = 30000;  ///< REQUESTED / CAUGHT with no command this long → back to the app (#364)

}  // namespace Boot
}  // namespace OpenSkyhawk
