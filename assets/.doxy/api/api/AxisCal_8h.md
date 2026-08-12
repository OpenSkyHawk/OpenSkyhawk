

# File AxisCal.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**AxisCal.h**](AxisCal_8h.md)

[Go to the source code of this file](AxisCal_8h_source.md)

_Per-axis HID calibration — storage layout, the two-segment transform, and CRC._ [More...](#detailed-description)

* `#include <stddef.h>`
* `#include <stdint.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |


## Classes

| Type | Name |
| ---: | :--- |
| struct | [**AxisCal**](structOpenSkyhawk_1_1AxisCal.md) <br>_Captured endpoints for one axis, unsigned 0–65535 throughout._  |
| struct | [**CalBlob**](structOpenSkyhawk_1_1CalBlob.md) <br>_The whole persisted calibration set, written and erased as one unit._  |


















































## Detailed Description


A hall-effect stick spans only part of the ADC range (57–62% on the bench rig), so an uncalibrated axis reaches roughly ±20k instead of ±32767. Its neutral position is also rarely at the midpoint of that span, which a single linear map cannot correct: stretching min..max fixes the range but leaves the stick resting off-centre.


This is the fix — three captured points (min, centre, max) and two linear segments, so centre lands exactly on 32768 regardless of how asymmetric the mechanical travel is.


Everything here is **pure**: no EEPROM, no Serial, no millis(), no globals, no hardware. That is deliberate. [**SimGateway**](namespaceSimGateway.md)'s `_hidSetAxis` is a no-op stub in SIMGATEWAY\_TEST builds, so a transform reachable only through HIDAxis::dispatch() would be unobservable in tests; a free function taking its inputs as arguments is callable directly from a test sketch.


Deliberately not wrapped in `#ifdef ARDUINO_ARCH_RP2040` — this is portable C++ and the guard would only obstruct compiling it anywhere else.




**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/AxisCal.h`

