

# File I2cHealth.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Helpers**](dir_9e93d9a1721bcf27b2030ff612e0fc11.md) **>** [**I2cHealth**](dir_741d33806df633606a48f25556e87791.md) **>** [**I2cHealth.h**](I2cHealth_8h.md)

[Go to the source code of this file](I2cHealth_8h_source.md)

_Circuit-breaker mixin ("trait") for I2C-backed device classes._ [More...](#detailed-description)

* `#include <Arduino.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**I2cHealth**](classOpenSkyhawk_1_1I2cHealth.md) <br>_Per-device I2C circuit breaker. Mix into any class that talks to an I2C device._  |


















































## Detailed Description


A blocking I2C transaction to an absent or dead device stalls `PanelGroup::loop()` and starves the node heartbeat — the bridge then flaps the node online/offline (#164). Any [**OpenSkyhawk**](namespaceOpenSkyhawk.md) class that drives an I2C device **mixes this in** and implements `i2cProbe()` (its own reachability check); it then gates every I2C op behind `i2cReachable()`. A dead device drops from "block every
loop" to "one probe every `I2C\_RETRY\_MS`", and auto-recovers when it returns. The class's data/decode path must stay I2C-free, so values stay current and the next reachable frame catches up to the live value instead of showing a stale one.


This is the C++ analog of a trait: shared behaviour lives here, the contract (`i2cProbe()`) is pure-virtual so a mixing class cannot compile without honouring it.




**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Helpers/I2cHealth/I2cHealth.h`

