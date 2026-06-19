

# File I2cMux.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Helpers**](dir_9e93d9a1721bcf27b2030ff612e0fc11.md) **>** [**I2cMux**](dir_b0e3ddf276daac85bddb20c46644a5c8.md) **>** [**I2cMux.h**](I2cMux_8h.md)

[Go to the source code of this file](I2cMux_8h_source.md)

_TCA9548A 1-to-8 I2C multiplexer channel selector for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _._[More...](#detailed-description)

* `#include <Arduino.h>`
* `#include <Wire.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br> |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**I2cMux**](classOpenSkyhawk_1_1I2cMux.md) <br>_Selects one downstream channel of a TCA9548A I2C multiplexer._  |


















































## Detailed Description


One I2cMux instance == one TCA9548A chip on one I2C bus. A DrumDisplay (or any future muxed I2C output) holds a pointer to an I2cMux plus a channel index and calls select() before every I2C transaction so the right downstream branch is live. The mux is a passive switch — the U8G2 driver still owns the device address; the mux only routes SDA/SCL.




**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Helpers/I2cMux/I2cMux.h`

