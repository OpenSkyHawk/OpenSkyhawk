

# File PanelGroup.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**PanelGroup.h**](PanelGroup_8h.md)

[Go to the source code of this file](PanelGroup_8h_source.md)

_CAN sub-node domain layer for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _panel boards._[More...](#detailed-description)

* `#include <Arduino.h>`
* `#include <Wire.h>`
* `#include <MCP23017.h>`
* `#include "ADS1115.h"`
* `#include "PinRef.h"`
* `#include <CANProtocol.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br> |
| namespace | [**PanelGroup**](namespacePanelGroup.md) <br>_Static singleton for CAN sub-node (_ [_**PanelGroup**_](namespacePanelGroup.md) _) firmware._ |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**InputBase**](classOpenSkyhawk_1_1InputBase.md) <br>_Abstract base for all hardware-polled input objects._  |
| class | [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) <br>_Abstract base for all DCS-driven output objects._  |


















































## Detailed Description


Provides the [**PanelGroup**](namespacePanelGroup.md) namespace (expander registration, setup, loop, MCP cache bridge) and the [**OpenSkyhawk**](namespaceOpenSkyhawk.md) base classes that input and output objects inherit from. All input/output objects are declared at global scope in a sketch so their constructors self-register before setup() runs.


Sketch pattern: 
```C++
#include <Wire.h>
#include <MCP23017.h>
#include <PanelGroup.h>
#include <LED.h>
#include <A4EC_OutputIds.h>

MCP23017 exp1(0x20, Wire);
ADS1115  adc1;

const PinRef PIN_MASTER_ARM  = PinRef(exp1, PORT_A, 3);
const PinRef PIN_CAUTION_LED = PinRef(PB0);

OpenSkyhawk::LED masterCaution(A_4E_C_MASTER_CAUTION, A_4E_C_MASTER_CAUTION_AM,
                               PIN_CAUTION_LED);

void setup() {
    Wire.begin();
    PanelGroup::registerExpander(exp1, PB3, PB4);  // INTA→PB3, INTB→PB4
    PanelGroup::registerADC(adc1, 0x48, Wire);
    PanelGroup::setup();
}
void loop() { PanelGroup::loop(); }
```





**Version:**

0.3.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PanelGroup.h`

