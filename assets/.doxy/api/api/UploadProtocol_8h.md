

# File UploadProtocol.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**UploadProtocol.h**](UploadProtocol_8h.md)

[Go to the source code of this file](UploadProtocol_8h_source.md)

_The Gateway uploader's USB contract (#366) — states, phases, results and payload layouts of the UPL\_\* messages._ [More...](#detailed-description)

* `#include <stdint.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |




















































## Detailed Description


The messages ride the calibration channel's envelope (`AA 53 4B 43`, TYPE, SEQ, LEN-exact, CRC16); their type codes and NACK reasons are in [**AxisCal.h**](AxisCal_8h.md) (`CAL_T_UPL_*`, `CAL_NACK_BUSY`…). The client streams an image, the Gateway stages and verifies it, then flashes a board's bootloader over CAN through [**PanelBridge**](namespacePanelBridge.md) — and finishes on its own if the client goes away. Behaviour: FirmwarePlan/07-simgateway-api.md § [**Uploader**](namespaceUploader.md); messages: FirmwarePlan/03-uart-usb-hid-protocol.md.


Every value is a named enum or constexpr so the client sync can read it (#346).




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/UploadProtocol.h`

