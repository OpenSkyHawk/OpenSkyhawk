

# File MotorDriver.h



[**FileList**](files.md) **>** [**Drivers**](dir_da1b6a20235952b69490534d482f5898.md) **>** [**MotorDriver**](dir_7cabaf4812e32c14ff26922d3804a645.md) **>** [**MotorDriver.h**](MotorDriver_8h.md)

[Go to the source code of this file](MotorDriver_8h_source.md)

_Abstract base for non-blocking motor/servo drivers._ [More...](#detailed-description)

* `#include <Arduino.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**MotorDriver**](classOpenSkyhawk_1_1MotorDriver.md) <br>_Common interface every motor/servo backend implements._  |


















































## Detailed Description


A MotorDriver moves a physical actuator (gauge stepper, geared stepper, RC servo) toward a commanded position without blocking the loop. It owns the low-level drive (coil energising / PWM), homing, and per-step timing; it knows nothing about DCS-BIOS. High-level controls — NeedleGauge (value → angle), and later a motorised DrumDisplay or trim indicator — _compose_ a MotorDriver and drive it, so no stepper/servo code is duplicated across controls.


A MotorDriver is NOT an OutputBase: it is owned and ticked (update()) by whatever control uses it, not registered on [**PanelGroup**](namespacePanelGroup.md)'s output list directly.


Concrete drivers: StepperMotor (now); ServoMotor / StepDirMotor (future siblings).




**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Drivers/MotorDriver/MotorDriver.h`

