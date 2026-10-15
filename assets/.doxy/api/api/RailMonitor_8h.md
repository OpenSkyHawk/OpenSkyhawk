

# File RailMonitor.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**RailMonitor.h**](RailMonitor_8h.md)

[Go to the source code of this file](RailMonitor_8h_source.md)

_PDU monitoring node — one supply rail: condition flags, window statistics, page packing (#216)._ [More...](#detailed-description)

* `#include <stdint.h>`
* `#include <NodeStatus.h>`
* `#include <CANProtocol.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |
| namespace | [**Pdu**](namespaceOpenSkyhawk_1_1Pdu.md) <br> |


## Classes

| Type | Name |
| ---: | :--- |
| struct | [**Latch**](structOpenSkyhawk_1_1Pdu_1_1Latch.md) <br>_A condition that must hold_ `setMs` _to raise and its clear condition hold_`clearMs` _to drop._ |
| class | [**RailMonitor**](classOpenSkyhawk_1_1RailMonitor.md) <br> |
| struct | [**Config**](structOpenSkyhawk_1_1RailMonitor_1_1Config.md) <br>_Per-rail limits._  |


















































## Detailed Description


Fed one converted sample per scan (rail mV, current mA). Keeps:
* condition flags (PduRailFlag) with set persistence, clear persistence and hysteresis, so a brief blip neither raises nor drops a flag;
* window statistics for its PDU\_RAIL\_\* page — average + minimum voltage, average current — reset each time the page is sent periodically;
* the peak current for the PDU\_STATUS page, reset each time that page is sent periodically. It is a FaultSource, so its worst condition reaches HEALTH\_n.faultId through aggregateFaults().




Hardware-free: the caller converts ADC counts (PduMath) and supplies the clock, so the logic is tested with injected values.




**Note:**

Static lifetime only (FaultSource registry rule) — construct as a global. 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/RailMonitor.h`

