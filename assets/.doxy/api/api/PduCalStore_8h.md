

# File PduCalStore.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**PduCalStore.h**](PduCalStore_8h.md)

[Go to the source code of this file](PduCalStore_8h_source.md)

_PDU per-board calibration in a reserved flash page — append-only log, newest valid wins (#216)._ [More...](#detailed-description)

* `#include <stdint.h>`
* `#include "PduConfig.h"`
* `#include "PduMath.h"`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |
| namespace | [**Pdu**](namespaceOpenSkyhawk_1_1Pdu.md) <br> |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**CalStore**](classOpenSkyhawk_1_1Pdu_1_1CalStore.md) <br>_The reserved flash page holding the calibration log._  |


















































## Detailed Description


The F103 has no EEPROM, so calibration lives in the last 1 KB flash page (PDU\_CAL\_PAGE\_ADDR), outside the firmware image: the PDU builds set `board_upload.maximum_size = 64512` so the build fails before code can grow into it, and an OpenOCD upload erases only the sectors it writes, so a reflash keeps the calibration.


The page holds up to 32 fixed 32-byte records. save() writes the next empty slot; load() returns the last slot whose magic and CRC check out. The page is erased only when it is full, so a power cut can lose at most the record being written — never the previous good one — except during the rare full-page erase.




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/PduCalStore.h`

