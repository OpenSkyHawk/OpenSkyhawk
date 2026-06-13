

# File SimGateway.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**SimGateway.h**](SimGateway_8h.md)

[Go to the source code of this file](SimGateway_8h_source.md)

_RP2040 USB HID gateway library for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) __[_**SimGateway**_](namespaceSimGateway.md) _board._[More...](#detailed-description)

* `#include <Arduino.h>`
* `#include <HIDControls.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br> |
| namespace | [**SimGateway**](namespaceSimGateway.md) <br> |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**HIDAxis**](classOpenSkyhawk_1_1HIDAxis.md) <br>_HID axis handler. Declared at sketch scope for each joystick axis._  |
| class | [**HIDButton**](classOpenSkyhawk_1_1HIDButton.md) <br>_HID button handler. Declared at sketch scope for each button._  |
| class | [**HIDHatSwitch**](classOpenSkyhawk_1_1HIDHatSwitch.md) <br>_HID hat switch handler. Declared at sketch scope for each hat switch._  |


















































## Detailed Description


Owns the USB CDC ↔ UART relay, 0xAA 0x55 HID frame demultiplexer, and OsJoystick.send() batching. HIDAxis, HIDButton, and HIDHatSwitch objects are declared in the sketch at file scope and self-register into linked lists at construction. [**SimGateway::loop()**](namespaceSimGateway.md#function-loop) walks those lists and dispatches matching HID frames to the OpenSkyhawkJoystick abstraction layer.


Does NOT run DCS-BIOS, parse DCS-BIOS addresses, or interact with CAN. Platform: RP2040 only.




**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/SimGateway.h`

