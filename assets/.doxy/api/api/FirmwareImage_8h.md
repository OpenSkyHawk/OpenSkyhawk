

# File FirmwareImage.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**FirmwareImage**](dir_3451fbe5395677fadb73167fe1a855e6.md) **>** [**FirmwareImage.h**](FirmwareImage_8h.md)

[Go to the source code of this file](FirmwareImage_8h_source.md)

_The image identity block every production STM32 image carries (#362)._ [More...](#detailed-description)

* `#include <stdint.h>`
* `#include <FirmwareVersion.h>`
* `#include <NodeStatus.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |
| namespace | [**FirmwareImage**](namespaceOpenSkyhawk_1_1FirmwareImage.md) <br> |


## Classes

| Type | Name |
| ---: | :--- |
| struct | [**Identity**](structOpenSkyhawk_1_1FirmwareImage_1_1Identity.md) <br> |

















































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**OSH\_FIRMWARE\_IDENTITY**](FirmwareImage_8h.md#define-osh_firmware_identity) (kind) `/* multi line expression */`<br>_Define this image's identity block. Exactly one per image — in the role library._  |

## Detailed Description


32 bytes, little-endian, at a fixed offset from the app's start (IDENTITY\_OFFSET). The bootloader (#363) checks it at every boot and before it commits an update; the Gateway uploader (#366) and the client (SkyHawkClient#177) read it from an image file before anything is sent. The shared linker script (Firmware/shared/stm32f103.ld) places the block; Firmware/shared/osh\_image.py fills in imageLength and imageCrc after linking. Everything else is set at compile time.


The block is defined by the role library — [**PanelGroup**](namespacePanelGroup.md), [**PduNode**](namespacePduNode.md), [**PanelBridge**](namespacePanelBridge.md) — with its NodeKind, through [**OSH\_FIRMWARE\_IDENTITY()**](FirmwareImage_8h.md#define-osh_firmware_identity). A sketch without a role has no block. 


    
## Macro Definition Documentation





### define OSH\_FIRMWARE\_IDENTITY 

_Define this image's identity block. Exactly one per image — in the role library._ 
```C++
#define OSH_FIRMWARE_IDENTITY (
    kind
) `/* multi line expression */`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/FirmwareImage/FirmwareImage.h`

