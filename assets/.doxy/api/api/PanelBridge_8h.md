

# File PanelBridge.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelBridge**](dir_f592a3c441b32532ba8eb6b28add2a90.md) **>** [**PanelBridge.h**](PanelBridge_8h.md)

[Go to the source code of this file](PanelBridge_8h_source.md)

_STM32 CAN master and DCS-BIOS processing node for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _._[More...](#detailed-description)

* `#include <stdint.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**PanelBridge**](namespacePanelBridge.md) <br> |



















































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**PANELBRIDGE\_NODE\_STATUS**](PanelBridge_8h.md#define-panelbridge_node_status)  `1`<br> |

## Detailed Description


[**PanelBridge**](namespacePanelBridge.md) bridges the RP2040 [**SimGateway**](namespaceSimGateway.md) (UART2, PA2/PA3) and the [**PanelGroup**](namespacePanelGroup.md) CAN cluster. It runs the DCS-BIOS library on Serial, batches DCS output to CAN CTRL\_BCAST frames, routes CAN EVTs to DCS-BIOS commands or HID frames, and tracks [**PanelGroup**](namespacePanelGroup.md) node liveness.


Minimal production sketch: 
```C++
#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>

void setup() {
    PanelBridge::setup();
    DcsBios::setup();
}
void loop() {
    DcsBios::loop();
    PanelBridge::loop();
}
```





**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    
## Macro Definition Documentation





### define PANELBRIDGE\_NODE\_STATUS 

```C++
#define PANELBRIDGE_NODE_STATUS `1`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelBridge/PanelBridge.h`

