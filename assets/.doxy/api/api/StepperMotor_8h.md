

# File StepperMotor.h



[**FileList**](files.md) **>** [**Drivers**](dir_da1b6a20235952b69490534d482f5898.md) **>** [**StepperMotor**](dir_f431add5022471a872df403ed217c535.md) **>** [**StepperMotor.h**](StepperMotor_8h.md)

[Go to the source code of this file](StepperMotor_8h_source.md)

_Non-blocking 4-wire stepper driver on_ [_**PinRef**_](classPinRef.md) _coils._[More...](#detailed-description)

* `#include <PanelGroup.h>`
* `#include <Drivers/MotorDriver/MotorDriver.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br> |


## Classes

| Type | Name |
| ---: | :--- |
| struct | [**AccelPoint**](structOpenSkyhawk_1_1AccelPoint.md) <br>_One point on the acceleration curve (SwitecX25 form)._  |
| struct | [**HomeSensor**](structOpenSkyhawk_1_1HomeSensor.md) <br>_Home-sensor parameters (_ HomeMode::SENSOR _only)._ |
| struct | [**StepperConfig**](structOpenSkyhawk_1_1StepperConfig.md) <br>_Full per-instance stepper configuration. Authored per sketch (panel wiring)._  |
| class | [**StepperMotor**](classOpenSkyhawk_1_1StepperMotor.md) <br>_Non-blocking instrument-gauge stepper driven through_ [_**PinRef**_](classPinRef.md) _coils._ |


















































## Detailed Description


A control-agnostic MotorDriver for instrument gauge steppers. Drives four coils through [**PinRef**](classPinRef.md), so the coils may be native STM32 GPIO **or** an MCP23017 expander with no code change. The motion engine is ported from Guy Carpenter's SwitecX25 library: an integer (no-FPU) table-driven trapezoidal accel/decel — a `vel` proxy ramps up one step at a time and decelerates once the steps remaining to target fall below `vel`, giving smooth acceleration into and out of every move.


One drive profile (StepPattern::SWITEC\_6STATE) covers the air-core instrument stepper family on hand — X27.589 / VID-29 / BKA-30 are the same motor electrically; a coil that runs reversed is corrected by swapping two constructor pins, not a new profile. StepPattern::FULL\_4STATE is reserved for generic geared 4-wire steppers.


Homing: HomeMode::STALL drives into a mechanical end-stop (no sensor); HomeMode::SENSOR seeks a debounced digital home sensor — a micro switch, reed, hall, or opto-interrupter all read identically through one [**PinRef**](classPinRef.md) + an active-level flag.




**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Drivers/StepperMotor/StepperMotor.h`

