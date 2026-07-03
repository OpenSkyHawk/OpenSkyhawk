

# File NeedleGauge.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Outputs**](dir_529c528362a647a34d31d0b3b420ca72.md) **>** [**NeedleGauge**](dir_61ced45d99aac20e353c7cae873553bb.md) **>** [**NeedleGauge.h**](NeedleGauge_8h.md)

[Go to the source code of this file](NeedleGauge_8h_source.md)

_Pointer-gauge output: maps one DCS-BIOS value to a motor position._ [More...](#detailed-description)

* `#include <PanelGroup.h>`
* `#include <Drivers/MotorDriver/MotorDriver.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |


## Classes

| Type | Name |
| ---: | :--- |
| struct | [**GaugeCal**](structOpenSkyhawk_1_1GaugeCal.md) <br>_Value → position calibration for one gauge._  |
| class | [**NeedleGauge**](classOpenSkyhawk_1_1NeedleGauge.md) <br>_DCS-driven pointer gauge over any_ [_**MotorDriver**_](classOpenSkyhawk_1_1MotorDriver.md) _backend._ |


















































## Detailed Description


A thin OutputBase that _composes_ a MotorDriver (StepperMotor today; a servo or step/dir driver later) and does only the gauge semantics — decode the 16-bit DCS-BIOS value, map it to a driver-native position (linear or piecewise-calibrated), and command the motor. All low-level drive, acceleration, and homing live in the MotorDriver, so 119 A-4E pointer gauges share one class over any backend.


Usage (per-sketch wiring): 
```C++
StepperMotor driftMotor(PinRef(PA0), PinRef(PA1), PinRef(PA4), PinRef(PA5), DRIFT_CFG);
NeedleGauge  drift(A_4E_C_APN153_DRIFT_GAUGE, A_4E_C_APN153_DRIFT_GAUGE_AM,
                   driftMotor, DRIFT_CAL);
```





**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Outputs/NeedleGauge/NeedleGauge.h`

