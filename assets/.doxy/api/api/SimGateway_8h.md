

# File SimGateway.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**SimGateway.h**](SimGateway_8h.md)

[Go to the source code of this file](SimGateway_8h_source.md)

_RP2040 USB HID + DCS-BIOS gateway domain layer for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _._[More...](#detailed-description)

* `#include <Arduino.h>`
* `#include <Joystick.h>`
* `#include <CANProtocol.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**SimGateway**](namespaceSimGateway.md) <br>_&lt; Exposed so sketches can call Joystick.button() etc._  |




















































## Detailed Description


Owns USB device identity, Joystick HID composite device, and the UART link to the [**PanelBridge**](namespacePanelBridge.md) STM32. Parses incoming DIAG frames from [**PanelBridge**](namespacePanelBridge.md) and dispatches them to registered callbacks. DCS-BIOS setup/loop must be called by the sketch — they require `#define DCSBIOS_DEFAULT_SERIAL` in the sketch's translation unit before `#include <DcsBios.h>`.


A minimal gateway sketch:



```C++
#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <SimGateway.h>

DcsBios::IntegerBuffer rpmBuf(A_4E_C_RPM, onRpmChange);

void setup() {
    SimGateway::setup(Serial1);
    DcsBios::setup();
}
void loop() {
    DcsBios::loop();
    SimGateway::loop();
}
```





**Note:**

Including [**SimGateway.h**](SimGateway_8h.md) pulls in Joystick.h transitively, so sketches can call Joystick.button() etc. without an extra include.




**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/SimGateway.h`

