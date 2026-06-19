

# Struct OpenSkyhawk::HomeSensor



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**HomeSensor**](structOpenSkyhawk_1_1HomeSensor.md)



_Home-sensor parameters (_ HomeMode::SENSOR _only)._[More...](#detailed-description)

* `#include <StepperMotor.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  bool | [**activeLow**](#variable-activelow)  <br>_true: sensor asserts LOW; false: asserts HIGH_  |
|  uint8\_t | [**debounceMs**](#variable-debouncems)  <br>_stable-assert confirmation window_  |
|  uint16\_t | [**maxSeekSteps**](#variable-maxseeksteps)  <br>_abort the seek after this many steps (mis-wired safety)_  |












































## Detailed Description


Any digital home detector reduces to a debounced level read; the sensor type is purely a wiring/polarity choice handled by `activeLow`. 


    
## Public Attributes Documentation




### variable activeLow 

_true: sensor asserts LOW; false: asserts HIGH_ 
```C++
bool OpenSkyhawk::HomeSensor::activeLow;
```




<hr>



### variable debounceMs 

_stable-assert confirmation window_ 
```C++
uint8_t OpenSkyhawk::HomeSensor::debounceMs;
```




<hr>



### variable maxSeekSteps 

_abort the seek after this many steps (mis-wired safety)_ 
```C++
uint16_t OpenSkyhawk::HomeSensor::maxSeekSteps;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Drivers/StepperMotor/StepperMotor.h`

