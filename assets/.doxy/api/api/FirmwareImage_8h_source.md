

# File FirmwareImage.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**FirmwareImage**](dir_3451fbe5395677fadb73167fe1a855e6.md) **>** [**FirmwareImage.h**](FirmwareImage_8h.md)

[Go to the documentation of this file](FirmwareImage_8h.md)


```C++
#pragma once
#include <stdint.h>
#include <FirmwareVersion.h>
#include <NodeStatus.h>   // NodeKind

namespace OpenSkyhawk {
namespace FirmwareImage {

constexpr uint32_t IDENTITY_OFFSET = 0x200;      
constexpr uint32_t MAGIC           = 0x594B534Fu; 
constexpr uint16_t LAYOUT_VERSION  = 1;
constexpr uint32_t UNSTAMPED       = 0xFFFFFFFFu; 

struct Identity {
    uint32_t magic;          
    uint16_t layoutVersion;  
    uint16_t blockSize;      
    uint32_t imageLength;    
    uint32_t imageCrc;       
    uint8_t  nodeId;         
    uint8_t  nodeKind;       
    uint8_t  fwMajor;        
    uint8_t  fwMinor;        
    uint8_t  fwPatch;        
    uint8_t  reserved[11];   
};
static_assert(sizeof(Identity) == 32, "the identity block is 32 bytes");

// Field offsets inside the block — for readers that work on raw bytes (client sync, uploader, tests).
constexpr uint32_t OFF_MAGIC = 0, OFF_LAYOUT = 4, OFF_SIZE = 6, OFF_LENGTH = 8, OFF_CRC = 12,
                   OFF_NODE_ID = 16, OFF_NODE_KIND = 17, OFF_FW_MAJOR = 18, OFF_FW_MINOR = 19, OFF_FW_PATCH = 20;

// imageCrc algorithm: CRC-32/MPEG-2 — poly 0x04C11DB7, init 0xFFFFFFFF, no reflection, no final XOR —
// over 32-bit little-endian words (what the F103's hardware CRC unit computes). Test vectors:
constexpr uint32_t CRC_VECTOR_WORD_12345678 = 0xDF8A8A2Bu;   
constexpr uint32_t CRC_VECTOR_BYTES_00_0F   = 0x081B46CAu;   

}  // namespace FirmwareImage
}  // namespace OpenSkyhawk

#define OSH_FIRMWARE_IDENTITY(kind)                                                                  \
    extern "C" __attribute__((section(".osh_identity"), used))                                       \
    const ::OpenSkyhawk::FirmwareImage::Identity osh_identity = {                                    \
        ::OpenSkyhawk::FirmwareImage::MAGIC, ::OpenSkyhawk::FirmwareImage::LAYOUT_VERSION,            \
        (uint16_t)sizeof(::OpenSkyhawk::FirmwareImage::Identity),                                     \
        ::OpenSkyhawk::FirmwareImage::UNSTAMPED, ::OpenSkyhawk::FirmwareImage::UNSTAMPED,             \
        (uint8_t)(NODE_ID), (uint8_t)(kind),                                                          \
        OPENSKYHAWK_FW_MAJOR, OPENSKYHAWK_FW_MINOR, OPENSKYHAWK_FW_PATCH, {0}                         \
    }
```


