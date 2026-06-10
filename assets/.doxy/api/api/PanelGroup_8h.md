

# File PanelGroup.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**PanelGroup.h**](PanelGroup_8h.md)

[Go to the source code of this file](PanelGroup_8h_source.md)

_CAN sub-node domain layer for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _panel boards._[More...](#detailed-description)

* `#include <Arduino.h>`
* `#include <STM32Board.h>`
* `#include <CANProtocol.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Output and input classes for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _panel boards._ |
| namespace | [**PanelGroup**](namespacePanelGroup.md) <br>_Static singleton for CAN sub-node (_ [_**PanelGroup**_](namespacePanelGroup.md) _) firmware._ |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**InputBase**](classOpenSkyhawk_1_1InputBase.md) <br>_Base class for all hardware-polled input objects on a_ [_**PanelGroup**_](namespacePanelGroup.md) _board._ |
| class | [**IntegerOutput**](classOpenSkyhawk_1_1IntegerOutput.md) <br>_Call an arbitrary function with the raw value from a ControlPacket._  |
| class | [**LED**](classOpenSkyhawk_1_1LED.md) <br>_Drive a GPIO pin from a single bit of a DCS-BIOS output value._  |
| class | [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) <br>_Base class for all DCS-driven output objects on a_ [_**PanelGroup**_](namespacePanelGroup.md) _board._ |
| class | [**Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md) <br>_Debounced 2-position GPIO switch — sends a ControlPacket CAN event on change._  |


















































## Detailed Description


Provides the [**PanelGroup**](namespacePanelGroup.md) singleton namespace and the [**OpenSkyhawk**](namespaceOpenSkyhawk.md) output and input object classes that panel sketches declare at global scope — mirroring the DCS-BIOS design pattern.


A production panel sketch looks like this:



```C++
#include <PanelGroup.h>

OpenSkyhawk::LED     armLed(A_4E_C_ARM_MASTER, 0x4000, PB0);
OpenSkyhawk::Switch2Pos ejSafe(A_4E_C_SEAT_EJECT_SAFE, PA1);

void setup() { STM32Board::setDebug(true); PanelGroup::setup(); }
void loop()  { PanelGroup::loop(); }
```





**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PanelGroup.h`

