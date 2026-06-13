

# File OpenSkyhawk.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**OpenSkyhawk.h**](OpenSkyhawk_8h.md)

[Go to the source code of this file](OpenSkyhawk_8h_source.md)

_Umbrella include for_ [_**PanelGroup**_](namespacePanelGroup.md) _sketch files._[More...](#detailed-description)

* `#include <STM32Board.h>`
* `#include <PanelGroup.h>`
* `#include <LED.h>`
* `#include <Switch2Pos.h>`
* `#include <A4EC_CmdIds.h>`
* `#include <A4EC_OutputIds.h>`

































































## Detailed Description


Include order matters: [**PanelGroup.h**](PanelGroup_8h.md) first (defines InputBase/OutputBase), then concrete classes (their [**PanelGroup.h**](PanelGroup_8h.md) re-include is a no-op via #pragma once). [**A4EC\_InputMap.h**](A4EC__InputMap_8h.md) is NOT included here — it is used only by [**PanelBridge.cpp**](PanelBridge_8cpp.md).




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/OpenSkyhawk.h`

