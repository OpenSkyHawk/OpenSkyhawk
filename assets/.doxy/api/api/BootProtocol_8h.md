

# File BootProtocol.h



[**FileList**](files.md) **>** [**BootProtocol**](dir_363c1f8b3b61005f0653a81dde367b4b.md) **>** [**BootProtocol.h**](BootProtocol_8h.md)

[Go to the source code of this file](BootProtocol_8h_source.md)

_The CAN bootloader's wire contract (#363) — frame IDs, commands, replies, status codes, the entry flag and the protocol version._ [More...](#detailed-description)

* `#include <stdint.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |
| namespace | [**Boot**](namespaceOpenSkyhawk_1_1Boot.md) <br> |


## Classes

| Type | Name |
| ---: | :--- |
| struct | [**BootloaderInfo**](structOpenSkyhawk_1_1Boot_1_1BootloaderInfo.md) <br> |

















































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**BOOT\_RSP\_MSG\_NAME**](BootProtocol_8h.md#define-boot_rsp_msg_name)  `"\_BOOT\_RSP"`<br> |

## Detailed Description


One source for every side of an update: the bootloader (Firmware/Bootloader), the running firmware that sends a node into it ([**STM32Board**](namespaceSTM32Board.md) / [**CANProtocol**](namespaceCANProtocol.md), #365), the bridge relay ([**PanelBridge**](namespacePanelBridge.md), #365), the Gateway uploader (#366) and the client sync. Header-only, no Arduino, no HAL — the bootloader builds bare-metal.


Every wire value is a named `enum class X : T { NAME = literal }` or `constexpr T NAME = literal` so the client sync can read it (#346). Full protocol: FirmwarePlan/12-bootloader.md.




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    
## Macro Definition Documentation





### define BOOT\_RSP\_MSG\_NAME 

```C++
#define BOOT_RSP_MSG_NAME `"_BOOT_RSP"`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/BootProtocol/BootProtocol.h`

