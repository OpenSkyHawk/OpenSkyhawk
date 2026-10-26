#pragma once
/**
 * @file FirmwareImage.h
 * @brief The image identity block every production STM32 image carries (#362).
 *
 * 32 bytes, little-endian, at a fixed offset from the app's start (IDENTITY_OFFSET). The bootloader
 * (#363) checks it at every boot and before it commits an update; the Gateway uploader (#366) and the
 * client (SkyHawkClient#177) read it from an image file before anything is sent. The shared linker
 * script (Firmware/shared/stm32f103.ld) places the block; Firmware/shared/osh_image.py fills in
 * imageLength and imageCrc after linking. Everything else is set at compile time.
 *
 * The block is defined by the role library — PanelGroup, PduNode, PanelBridge — with its NodeKind,
 * through OSH_FIRMWARE_IDENTITY(). A sketch without a role has no block.
 */
#include <stdint.h>
#include <FirmwareVersion.h>
#include <NodeStatus.h>   // NodeKind

namespace OpenSkyhawk {
namespace FirmwareImage {

constexpr uint32_t IDENTITY_OFFSET = 0x200;      ///< from the app's start (the vector table's address)
constexpr uint32_t MAGIC           = 0x594B534Fu; ///< "OSKY" in memory order
constexpr uint16_t LAYOUT_VERSION  = 1;
constexpr uint32_t UNSTAMPED       = 0xFFFFFFFFu; ///< imageLength / imageCrc before osh_image.py runs

struct Identity {
    uint32_t magic;          ///< 0  MAGIC
    uint16_t layoutVersion;  ///< 4  LAYOUT_VERSION
    uint16_t blockSize;      ///< 6  sizeof(Identity); a newer layout may grow — readers parse the prefix they know
    uint32_t imageLength;    ///< 8  bytes from the app's start, padded to 4 — stamped after linking
    uint32_t imageCrc;       ///< 12 CRC-32/MPEG-2 over imageLength bytes, this word skipped — stamped after linking
    uint8_t  nodeId;         ///< 16 NODE_ID (the build flag, D10)
    uint8_t  nodeKind;       ///< 17 NodeKind of the role
    uint8_t  fwMajor;        ///< 18 FirmwareVersion.h — the same version READY_n reports
    uint8_t  fwMinor;        ///< 19
    uint8_t  fwPatch;        ///< 20
    uint8_t  reserved[11];   ///< 21 0
};
static_assert(sizeof(Identity) == 32, "the identity block is 32 bytes");

// Field offsets inside the block — for readers that work on raw bytes (client sync, uploader, tests).
constexpr uint32_t OFF_MAGIC = 0, OFF_LAYOUT = 4, OFF_SIZE = 6, OFF_LENGTH = 8, OFF_CRC = 12,
                   OFF_NODE_ID = 16, OFF_NODE_KIND = 17, OFF_FW_MAJOR = 18, OFF_FW_MINOR = 19, OFF_FW_PATCH = 20;

// imageCrc algorithm: CRC-32/MPEG-2 — poly 0x04C11DB7, init 0xFFFFFFFF, no reflection, no final XOR —
// over 32-bit little-endian words (what the F103's hardware CRC unit computes). Test vectors:
constexpr uint32_t CRC_VECTOR_WORD_12345678 = 0xDF8A8A2Bu;   ///< one word, 0x12345678
constexpr uint32_t CRC_VECTOR_BYTES_00_0F   = 0x081B46CAu;   ///< bytes 0x00..0x0F (four words)

}  // namespace FirmwareImage
}  // namespace OpenSkyhawk

/// Define this image's identity block. Exactly one per image — in the role library.
#define OSH_FIRMWARE_IDENTITY(kind)                                                                  \
    extern "C" __attribute__((section(".osh_identity"), used))                                       \
    const ::OpenSkyhawk::FirmwareImage::Identity osh_identity = {                                    \
        ::OpenSkyhawk::FirmwareImage::MAGIC, ::OpenSkyhawk::FirmwareImage::LAYOUT_VERSION,            \
        (uint16_t)sizeof(::OpenSkyhawk::FirmwareImage::Identity),                                     \
        ::OpenSkyhawk::FirmwareImage::UNSTAMPED, ::OpenSkyhawk::FirmwareImage::UNSTAMPED,             \
        (uint8_t)(NODE_ID), (uint8_t)(kind),                                                          \
        OPENSKYHAWK_FW_MAJOR, OPENSKYHAWK_FW_MINOR, OPENSKYHAWK_FW_PATCH, {0}                         \
    }
