

# Class OpenSkyhawk::StepperMotor



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**StepperMotor**](classOpenSkyhawk_1_1StepperMotor.md)



_Non-blocking instrument-gauge stepper driven through_ [_**PinRef**_](classPinRef.md) _coils._

* `#include <StepperMotor.h>`



Inherits the following classes: [OpenSkyhawk::MotorDriver](classOpenSkyhawk_1_1MotorDriver.md)






















































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**StepperMotor**](#function-steppermotor) ([**PinRef**](classPinRef.md) c1, [**PinRef**](classPinRef.md) c2, [**PinRef**](classPinRef.md) c3, [**PinRef**](classPinRef.md) c4, const [**StepperConfig**](structOpenSkyhawk_1_1StepperConfig.md) & cfg, [**PinRef**](classPinRef.md) homeSense=[**PinRef**](classPinRef.md)(), [**PinRef**](classPinRef.md) sleepEn=[**PinRef**](classPinRef.md)()) <br>_Construct a stepper over four coil pins._  |
| virtual void | [**configure**](#function-configure) () override<br>_coils OUTPUT, ~SLEEP HIGH, sensor INPUT, energise._  |
| virtual void | [**home**](#function-home) () override<br>_blocking homing (STALL or SENSOR), then park._  |
|  bool | [**homed**](#function-homed) () const<br>_True once homing has completed successfully (false if a SENSOR seek aborted)._  |
| virtual void | [**moveTo**](#function-moveto) (int32\_t pos) override<br>_retarget (clamp/wrap + deadband); non-blocking._  |
| virtual int32\_t | [**position**](#function-position) () override const<br>_current step (wrapped to 0..stepsPerRev if wrap)._  |
| virtual void | [**update**](#function-update) () override<br>_step toward target if due; auto-recal._  |


## Public Functions inherited from OpenSkyhawk::MotorDriver

See [OpenSkyhawk::MotorDriver](classOpenSkyhawk_1_1MotorDriver.md)

| Type | Name |
| ---: | :--- |
| virtual void | [**configure**](classOpenSkyhawk_1_1MotorDriver.md#function-configure) () = 0<br>_Configure pins / drive hardware. Call once from the owner's_ [_**configure()**_](classOpenSkyhawk_1_1MotorDriver.md#function-configure) _._ |
| virtual void | [**home**](classOpenSkyhawk_1_1MotorDriver.md#function-home) () = 0<br>_Establish the zero reference (mechanical stop, home sensor, or absolute read)._  |
| virtual void | [**moveTo**](classOpenSkyhawk_1_1MotorDriver.md#function-moveto) (int32\_t pos) = 0<br>_Set the target position in driver-native units. Non-blocking._  |
| virtual int32\_t | [**position**](classOpenSkyhawk_1_1MotorDriver.md#function-position) () const = 0<br>_Current position in driver-native units._  |
| virtual void | [**update**](classOpenSkyhawk_1_1MotorDriver.md#function-update) () = 0<br>_Advance one step/increment toward the target if due. Call every loop()._  |
| virtual  | [**~MotorDriver**](classOpenSkyhawk_1_1MotorDriver.md#function-motordriver) () = default<br> |






















































## Public Functions Documentation




### function StepperMotor 

_Construct a stepper over four coil pins._ 
```C++
OpenSkyhawk::StepperMotor::StepperMotor (
    PinRef c1,
    PinRef c2,
    PinRef c3,
    PinRef c4,
    const StepperConfig & cfg,
    PinRef homeSense=PinRef (),
    PinRef sleepEn=PinRef ()
) 
```





**Parameters:**


* `c1..c4` Coil PinRefs (GPIO or MCP23017). Swap two to reverse direction. 
* `cfg` Per-instance configuration (copied; cfg.accel must outlive this). 
* `homeSense` Home-sensor [**PinRef**](classPinRef.md) for HomeMode::SENSOR (NC default for STALL). 
* `sleepEn` Optional driver ~SLEEP/enable [**PinRef**](classPinRef.md), driven HIGH in [**configure()**](classOpenSkyhawk_1_1StepperMotor.md#function-configure). 




        

<hr>



### function configure 

_coils OUTPUT, ~SLEEP HIGH, sensor INPUT, energise._ 
```C++
virtual void OpenSkyhawk::StepperMotor::configure () override
```



Implements [*OpenSkyhawk::MotorDriver::configure*](classOpenSkyhawk_1_1MotorDriver.md#function-configure)


<hr>



### function home 

_blocking homing (STALL or SENSOR), then park._ 
```C++
virtual void OpenSkyhawk::StepperMotor::home () override
```



Implements [*OpenSkyhawk::MotorDriver::home*](classOpenSkyhawk_1_1MotorDriver.md#function-home)


<hr>



### function homed 

_True once homing has completed successfully (false if a SENSOR seek aborted)._ 
```C++
inline bool OpenSkyhawk::StepperMotor::homed () const
```




<hr>



### function moveTo 

_retarget (clamp/wrap + deadband); non-blocking._ 
```C++
virtual void OpenSkyhawk::StepperMotor::moveTo (
    int32_t pos
) override
```



Implements [*OpenSkyhawk::MotorDriver::moveTo*](classOpenSkyhawk_1_1MotorDriver.md#function-moveto)


<hr>



### function position 

_current step (wrapped to 0..stepsPerRev if wrap)._ 
```C++
virtual int32_t OpenSkyhawk::StepperMotor::position () override const
```



Implements [*OpenSkyhawk::MotorDriver::position*](classOpenSkyhawk_1_1MotorDriver.md#function-position)


<hr>



### function update 

_step toward target if due; auto-recal._ 
```C++
virtual void OpenSkyhawk::StepperMotor::update () override
```



Implements [*OpenSkyhawk::MotorDriver::update*](classOpenSkyhawk_1_1MotorDriver.md#function-update)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Drivers/StepperMotor/StepperMotor.h`

