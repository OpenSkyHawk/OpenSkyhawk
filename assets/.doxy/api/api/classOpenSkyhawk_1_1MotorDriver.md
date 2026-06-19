

# Class OpenSkyhawk::MotorDriver



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**MotorDriver**](classOpenSkyhawk_1_1MotorDriver.md)



_Common interface every motor/servo backend implements._ [More...](#detailed-description)

* `#include <MotorDriver.h>`





Inherited by the following classes: [OpenSkyhawk::StepperMotor](classOpenSkyhawk_1_1StepperMotor.md)
































## Public Functions

| Type | Name |
| ---: | :--- |
| virtual void | [**configure**](#function-configure) () = 0<br>_Configure pins / drive hardware. Call once from the owner's_ [_**configure()**_](classOpenSkyhawk_1_1MotorDriver.md#function-configure) _._ |
| virtual void | [**home**](#function-home) () = 0<br>_Establish the zero reference (mechanical stop, home sensor, or absolute read)._  |
| virtual void | [**moveTo**](#function-moveto) (int32\_t pos) = 0<br>_Set the target position in driver-native units. Non-blocking._  |
| virtual int32\_t | [**position**](#function-position) () const = 0<br>_Current position in driver-native units._  |
| virtual void | [**update**](#function-update) () = 0<br>_Advance one step/increment toward the target if due. Call every loop()._  |
| virtual  | [**~MotorDriver**](#function-motordriver) () = default<br> |




























## Detailed Description


Positions are in driver-native units (steps for a stepper, microseconds for a servo). The owning control maps DCS-BIOS values into that space. 


    
## Public Functions Documentation




### function configure 

_Configure pins / drive hardware. Call once from the owner's_ [_**configure()**_](classOpenSkyhawk_1_1MotorDriver.md#function-configure) _._
```C++
virtual void OpenSkyhawk::MotorDriver::configure () = 0
```





**Note:**

Runs after bus/board init ([**PanelGroup::setup()**](namespacePanelGroup.md#function-setup)), never from a constructor. 





        

<hr>



### function home 

_Establish the zero reference (mechanical stop, home sensor, or absolute read)._ 
```C++
virtual void OpenSkyhawk::MotorDriver::home () = 0
```





**Note:**

May block (boot-time homing). Call once, after [**configure()**](classOpenSkyhawk_1_1MotorDriver.md#function-configure). 





        

<hr>



### function moveTo 

_Set the target position in driver-native units. Non-blocking._ 
```C++
virtual void OpenSkyhawk::MotorDriver::moveTo (
    int32_t pos
) = 0
```





**Parameters:**


* `pos` Target position; the driver clamps / wraps to its own limits. 




        

<hr>



### function position 

_Current position in driver-native units._ 
```C++
virtual int32_t OpenSkyhawk::MotorDriver::position () const = 0
```




<hr>



### function update 

_Advance one step/increment toward the target if due. Call every loop()._ 
```C++
virtual void OpenSkyhawk::MotorDriver::update () = 0
```





**Note:**

Non-blocking — does at most the work for the current instant. 





        

<hr>



### function ~MotorDriver 

```C++
virtual OpenSkyhawk::MotorDriver::~MotorDriver () = default
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Drivers/MotorDriver/MotorDriver.h`

