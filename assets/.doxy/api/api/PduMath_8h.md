

# File PduMath.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**PduMath.h**](PduMath_8h.md)

[Go to the source code of this file](PduMath_8h_source.md)

_PDU monitoring node — pure conversions, no hardware access (#216)._ [More...](#detailed-description)

* `#include <stdint.h>`
* `#include <stddef.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |
| namespace | [**Pdu**](namespaceOpenSkyhawk_1_1Pdu.md) <br> |


## Classes

| Type | Name |
| ---: | :--- |
| struct | [**CalValues**](structOpenSkyhawk_1_1Pdu_1_1CalValues.md) <br>_Calibration values the conversions need. Defaults = uncalibrated._  |


















































## Detailed Description


Everything here is a function of its arguments, so it is tested with injected values on any target. ADC counts are 16-bit (analogReadResolution(16)); VDDA is in mV.




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/PduMath.h`

