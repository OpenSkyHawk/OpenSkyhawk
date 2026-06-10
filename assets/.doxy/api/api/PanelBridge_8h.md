

# File PanelBridge.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelBridge**](dir_f592a3c441b32532ba8eb6b28add2a90.md) **>** [**PanelBridge.h**](PanelBridge_8h.md)

[Go to the source code of this file](PanelBridge_8h_source.md)

_CAN master / UART bridge domain layer for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _._[More...](#detailed-description)

* `#include <Arduino.h>`
* `#include <STM32Board.h>`
* `#include <CANProtocol.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**PanelBridge**](namespacePanelBridge.md) <br>_Static singleton for CAN master / UART bridge firmware._  |




















































## Detailed Description


Provides a transparent bridge between the RP2040 [**SimGateway**](namespaceSimGateway.md) (over UART) and the CAN sub-nodes ([**PanelGroup**](namespacePanelGroup.md) boards). ControlPackets received from the RP2040 are broadcast on the CAN bus; frames received from sub-nodes are forwarded to the RP2040 as DIAG frames. Sub-node heartbeat watchdog fires callbacks on liveness changes.


A minimal master sketch:



```C++
#include <PanelBridge.h>

void setup() {
    STM32Board::setDebug(true);
    PanelBridge::setup(Serial2);   // UART2 PA2/PA3 @ 250000
}
void loop() { PanelBridge::loop(); }
```





**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelBridge/PanelBridge.h`

