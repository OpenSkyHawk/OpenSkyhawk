

# File BoardThermal.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**BoardThermal.h**](BoardThermal_8h.md)

[Go to the source code of this file](BoardThermal_8h_source.md)

_PDU monitoring node — board temperature from the NTC: warn / critical / sensor fault (#216)._ [More...](#detailed-description)

* `#include <stdint.h>`
* `#include <NodeStatus.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**BoardThermal**](classOpenSkyhawk_1_1BoardThermal.md) <br> |


















































## Detailed Description


Fed a converted reading (0.1 °C, from PduMath::ntcTempDC) roughly every 100 ms. Thresholds use hysteresis only — board temperature moves slowly, so no persistence timers. A FaultSource: critical → OVER\_TEMPERATURE, an open/shorted NTC → SENSOR\_FAULT. TEMP\_WARN is a telemetry flag only, not a node fault.


This is the PDU's real board temperature (±0.5 °C class NTC between the 12 V and 5 V pours). The MCU die temperature in HEALTH\_n is a separate, uncalibrated reading.




**Note:**

Static lifetime only (FaultSource registry rule) — construct as a global. 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/BoardThermal.h`

