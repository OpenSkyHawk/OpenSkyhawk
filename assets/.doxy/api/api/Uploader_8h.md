

# File Uploader.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**Uploader.h**](Uploader_8h.md)

[Go to the source code of this file](Uploader_8h_source.md)

_The Gateway's firmware uploader (#366) — stages an image the client streams over USB, then flashes a board's bootloader over CAN through_ [_**PanelBridge**_](namespacePanelBridge.md) _, on its own._[More...](#detailed-description)

* `#include <stdint.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**Uploader**](namespaceUploader.md) <br> |


## Classes

| Type | Name |
| ---: | :--- |
| struct | [**Io**](structUploader_1_1Io.md) <br>_How the uploader reaches the client and_ [_**PanelBridge**_](namespacePanelBridge.md) _._ |


















































## Detailed Description


Library-internal: [**SimGateway.cpp**](SimGateway_8cpp.md) feeds it the client's UPL\_\* frames, every line [**PanelBridge**](namespacePanelBridge.md) sends the host, and a tick from loop(); the uploader answers through the callbacks in [**Uploader::Io**](structUploader_1_1Io.md). Portable C++ — no Arduino, no globals of the caller's — so it can be exercised off-target.


Contract: FirmwarePlan/07-simgateway-api.md § [**Uploader**](namespaceUploader.md), [**UploadProtocol.h**](UploadProtocol_8h.md), [**BootProtocol.h**](BootProtocol_8h.md).




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/Uploader.h`

