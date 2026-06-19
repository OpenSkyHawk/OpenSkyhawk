

# File DrumDisplay.cpp



[**FileList**](files.md) **>** [**DrumDisplay**](dir_b8ac8c4bf654b399fff34fe5003d7d6c.md) **>** [**DrumDisplay.cpp**](DrumDisplay_8cpp.md)

[Go to the source code of this file](DrumDisplay_8cpp_source.md)

[More...](#detailed-description)

* `#include "DrumDisplay.h"`
* `#include <math.h>`
* `#include <string.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br> |




















































## Detailed Description




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE


Renderer (drawTape / drawFlag / per-place ease / per-cell clip) ported from the bench-verified SH1106 prototype. The class adds three things the prototype lacked: DCS-BIOS address→digit decode (onControlPacket), snap-then-settle (update), and descriptor-driven geometry auto-fit (fitGeometry) that replaces the prototype's hardcoded PX\_PER\_MM / COLW / GAP / FLAGW / CELLH constants. 


    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/DrumDisplay/DrumDisplay.cpp`

