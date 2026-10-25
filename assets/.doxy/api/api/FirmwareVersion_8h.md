

# File FirmwareVersion.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**FirmwareVersion**](dir_cb5acd30e4dc0e08f694c9971e69b791.md) **>** [**FirmwareVersion.h**](FirmwareVersion_8h.md)

[Go to the source code of this file](FirmwareVersion_8h_source.md)

_The firmware release version — one source for every board that reports it._ [More...](#detailed-description)

































































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**OPENSKYHAWK\_FW\_MAJOR**](FirmwareVersion_8h.md#define-openskyhawk_fw_major)  `0`<br> |
| define  | [**OPENSKYHAWK\_FW\_MINOR**](FirmwareVersion_8h.md#define-openskyhawk_fw_minor)  `1`<br> |
| define  | [**OPENSKYHAWK\_FW\_PATCH**](FirmwareVersion_8h.md#define-openskyhawk_fw_patch)  `0`<br> |

## Detailed Description


Every board is built from the same release, so there is one version, not one per library. It goes on the wire in two places: [**SimGateway**](namespaceSimGateway.md)'s calibration HELLO\_ACK, and each STM32 node's READY\_n payload (#295). release-please rewrites the three numbers below when a firmware release PR merges (generic updater, `x-release-please-*` markers; see release-please-config.json) — never edit them by hand.


Shared between STM32 and RP2040. Contains only `#define` constants.




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    
## Macro Definition Documentation





### define OPENSKYHAWK\_FW\_MAJOR 

```C++
#define OPENSKYHAWK_FW_MAJOR `0`
```




<hr>



### define OPENSKYHAWK\_FW\_MINOR 

```C++
#define OPENSKYHAWK_FW_MINOR `1`
```




<hr>



### define OPENSKYHAWK\_FW\_PATCH 

```C++
#define OPENSKYHAWK_FW_PATCH `0`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/FirmwareVersion/FirmwareVersion.h`

