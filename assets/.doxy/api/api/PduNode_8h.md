

# File PduNode.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**PduNode.h**](PduNode_8h.md)

[Go to the source code of this file](PduNode_8h_source.md)

_PDU monitoring node — the whole node in two calls (#216)._ [More...](#detailed-description)

* `#include <stdint.h>`
* `#include <STM32Board.h>`
* `#include <CANProtocol.h>`
* `#include "RailMonitor.h"`
* `#include "BoardThermal.h"`
* `#include "PduCalStore.h"`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**PduNode**](namespacePduNode.md) <br> |




















































## Detailed Description


A PDU sketch is just: 
```C++
#include <PduNode.h>
void setup() { STM32Board::setDebug(true); PduNode::setup(); }
void loop()  { PduNode::loop(); }
```
 with `-DNODE_ID=N` in platformio.ini. What runs:
* **Node status** — HB\_n / HEALTH\_n (board kind PDU, #344) / master-loss watchdog via [**CANProtocol::serviceNodeStatus()**](namespaceCANProtocol.md#function-servicenodestatus). Rail and thermal faults reach HEALTH\_n.faultId through the FaultSource registry.
* **Sampling** — the four rail channels every PDU\_SAMPLE\_US (1 kHz target); the NTC and Vrefint every 100 ms. Faults are held clear for PDU\_BOOT\_BLANK\_MS after boot.
* **Telemetry** — one TELEM\_n frame per second, rotating PDU\_RAIL\_12V → PDU\_RAIL\_5V → PDU\_STATUS; a page whose flags change goes out at once (≤ 1 per 100 ms per page); SYNC\_REQ sends all three pages immediately.
* **Calibration** — DiagSerial (115200) commands, one per line: `CAL SHOW` · `CAL VDDA <mV>` (DMM reading of the 3V3 rail) · `CAL ZERO` (no load connected) · `CAL CLEAR` · `RAW` (live readings + scan rate). Stored per board in the reserved flash page (PduCalStore).




Rail faults are reported to the host only (TELEM\_n flags + HEALTH\_n.faultId). The status LEDs follow every other node — CAN / link state plus the master-loss WARNING — and don't change for rail faults (FirmwarePlan D14). The board's power LEDs (D3 12 V / D4 5 V / D5 3V3) are passive, wired to their rails, and show rail presence only — this firmware never drives them.




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/PduNode.h`

