

# Struct OpenSkyhawk::StepperConfig



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**StepperConfig**](structOpenSkyhawk_1_1StepperConfig.md)



_Full per-instance stepper configuration. Authored per sketch (panel wiring)._ [More...](#detailed-description)

* `#include <StepperMotor.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  const [**AccelPoint**](structOpenSkyhawk_1_1AccelPoint.md) \* | [**accel**](#variable-accel)  <br>_acceleration curve (not owned)_  |
|  uint8\_t | [**accelN**](#variable-acceln)  <br>_entries in accel[]_  |
|  bool | [**autoRecal**](#variable-autorecal)  <br>_re-zero to homePosition when the sensor next asserts_  |
|  uint8\_t | [**deadband**](#variable-deadband)  <br>_ignore target changes within this many steps_  |
|  [**HomeMode**](namespaceOpenSkyhawk.md#enum-homemode) | [**home**](#variable-home)  <br>_homing strategy_  |
|  int16\_t | [**homePosition**](#variable-homeposition)  <br>_step index assigned at the home reference_  |
|  bool | [**homeSeekClockwise**](#variable-homeseekclockwise)  <br>_direction to seek the home reference_  |
|  uint16\_t | [**homeStepUs**](#variable-homestepus)  <br>_homing seek rate µs/step; MUST stay under the motor start-stop rate or the seek slips. 0 → library default (2000)_  |
|  int16\_t | [**maxPos**](#variable-maxpos)  <br>_upper travel clamp for moveTo (ignored if wrap)_  |
|  int16\_t | [**minPos**](#variable-minpos)  <br>_lower travel clamp for moveTo (ignored if wrap)_  |
|  int16\_t | [**parkPosition**](#variable-parkposition)  <br>_step to rest at after homing_  |
|  [**StepPattern**](namespaceOpenSkyhawk.md#enum-steppattern) | [**pattern**](#variable-pattern)  <br>_coil drive sequence_  |
|  uint16\_t | [**rangeSteps**](#variable-rangesteps)  <br>_mechanical stop-to-stop travel in steps; STALL home drives this (+margin). 0 → stepsPerRev_  |
|  uint32\_t | [**recalDebounceMs**](#variable-recaldebouncems)  <br>_minimum interval between auto-recals_  |
|  [**HomeSensor**](structOpenSkyhawk_1_1HomeSensor.md) | [**sensor**](#variable-sensor)  <br>_SENSOR-mode params (ignored for STALL)_  |
|  uint16\_t | [**stepsPerRev**](#variable-stepsperrev)  <br>_steps per full revolution (calibrate empirically)_  |
|  bool | [**wrap**](#variable-wrap)  <br>_continuous-rotation gauge (shortest-path, no clamp)_  |












































## Detailed Description




**Note:**

`accel` must point at storage that outlives the [**StepperMotor**](classOpenSkyhawk_1_1StepperMotor.md) (e.g. a static const table). `homePosition` / `parkPosition` / `minPos` / `maxPos` are in steps; a sketch may compute them from degrees as `round`(deg\*stepsPerRev/360). 





    
## Public Attributes Documentation




### variable accel 

_acceleration curve (not owned)_ 
```C++
const AccelPoint* OpenSkyhawk::StepperConfig::accel;
```




<hr>



### variable accelN 

_entries in accel[]_ 
```C++
uint8_t OpenSkyhawk::StepperConfig::accelN;
```




<hr>



### variable autoRecal 

_re-zero to homePosition when the sensor next asserts_ 
```C++
bool OpenSkyhawk::StepperConfig::autoRecal;
```




<hr>



### variable deadband 

_ignore target changes within this many steps_ 
```C++
uint8_t OpenSkyhawk::StepperConfig::deadband;
```




<hr>



### variable home 

_homing strategy_ 
```C++
HomeMode OpenSkyhawk::StepperConfig::home;
```




<hr>



### variable homePosition 

_step index assigned at the home reference_ 
```C++
int16_t OpenSkyhawk::StepperConfig::homePosition;
```




<hr>



### variable homeSeekClockwise 

_direction to seek the home reference_ 
```C++
bool OpenSkyhawk::StepperConfig::homeSeekClockwise;
```




<hr>



### variable homeStepUs 

_homing seek rate µs/step; MUST stay under the motor start-stop rate or the seek slips. 0 → library default (2000)_ 
```C++
uint16_t OpenSkyhawk::StepperConfig::homeStepUs;
```




<hr>



### variable maxPos 

_upper travel clamp for moveTo (ignored if wrap)_ 
```C++
int16_t OpenSkyhawk::StepperConfig::maxPos;
```




<hr>



### variable minPos 

_lower travel clamp for moveTo (ignored if wrap)_ 
```C++
int16_t OpenSkyhawk::StepperConfig::minPos;
```




<hr>



### variable parkPosition 

_step to rest at after homing_ 
```C++
int16_t OpenSkyhawk::StepperConfig::parkPosition;
```




<hr>



### variable pattern 

_coil drive sequence_ 
```C++
StepPattern OpenSkyhawk::StepperConfig::pattern;
```




<hr>



### variable rangeSteps 

_mechanical stop-to-stop travel in steps; STALL home drives this (+margin). 0 → stepsPerRev_ 
```C++
uint16_t OpenSkyhawk::StepperConfig::rangeSteps;
```




<hr>



### variable recalDebounceMs 

_minimum interval between auto-recals_ 
```C++
uint32_t OpenSkyhawk::StepperConfig::recalDebounceMs;
```




<hr>



### variable sensor 

_SENSOR-mode params (ignored for STALL)_ 
```C++
HomeSensor OpenSkyhawk::StepperConfig::sensor;
```




<hr>



### variable stepsPerRev 

_steps per full revolution (calibrate empirically)_ 
```C++
uint16_t OpenSkyhawk::StepperConfig::stepsPerRev;
```




<hr>



### variable wrap 

_continuous-rotation gauge (shortest-path, no clamp)_ 
```C++
bool OpenSkyhawk::StepperConfig::wrap;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Drivers/StepperMotor/StepperMotor.h`

