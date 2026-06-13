

# File PinRef.cpp



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**PinRef.cpp**](PinRef_8cpp.md)

[Go to the source code of this file](PinRef_8cpp_source.md)



* `#include "PinRef.h"`
* `#include "ADS1115.h"`
* `#include <MCP23017.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**PanelGroup**](namespacePanelGroup.md) <br>_Static singleton for CAN sub-node (_ [_**PanelGroup**_](namespacePanelGroup.md) _) firmware._ |








## Public Attributes

| Type | Name |
| ---: | :--- |
|  const [**PinRef**](classPinRef.md) | [**PIN\_NC**](#variable-pin_nc)  <br>_No-connect sentinel. Pass where no physical pin exists._  |












































## Public Attributes Documentation




### variable PIN\_NC 

_No-connect sentinel. Pass where no physical pin exists._ 
```C++
const PinRef PIN_NC;
```



All reads return false / 0; all writes are no-ops. Provided as a named constant for readability in wiring maps.


Usage: `PinRef pins[] = { PinRef(exp1, PORT_A, 0) , PIN_NC, PinRef(exp1, PORT_A, 2) };`


See also: ANALOG\_NC (0xFFFF) for AnalogMultiPos unused voltage levels. 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PinRef.cpp`

