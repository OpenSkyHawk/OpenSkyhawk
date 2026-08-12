

# Struct OpenSkyhawk::AxisCal



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**AxisCal**](structOpenSkyhawk_1_1AxisCal.md)



_Captured endpoints for one axis, unsigned 0–65535 throughout._ [More...](#detailed-description)

* `#include <AxisCal.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint16\_t | [**centre**](#variable-centre)  <br>_Raw value at rest. Captured, never computed._  |
|  uint16\_t | [**deadzone**](#variable-deadzone)  <br> |
|  uint16\_t | [**max**](#variable-max)  <br>_Raw value at the axis's high mechanical stop._  |
|  uint16\_t | [**min**](#variable-min)  <br>_Raw value at the axis's low mechanical stop._  |












































## Detailed Description


`centre` is always _provided_ data, never derived — the client captures it, and neither the device nor the client substitutes `(min + max) / 2`. Raw midpoint is not physical midpoint whenever the transfer is nonlinear, and for a rotating-magnet hall axis it is nonlinear by construction (the normal field component goes as sin θ). An offset centre is the normal case and is exactly what the second segment exists to absorb. 


    
## Public Attributes Documentation




### variable centre 

_Raw value at rest. Captured, never computed._ 
```C++
uint16_t OpenSkyhawk::AxisCal::centre;
```




<hr>



### variable deadzone 

```C++
uint16_t OpenSkyhawk::AxisCal::deadzone;
```



Reserved, always 0 in blob version 1. The field exists only so that adding a deadzone later needs no CAL\_VERSION bump; nothing reads it, and the wire protocol rejects a non-zero value. [**AnalogInput**](classOpenSkyhawk_1_1AnalogInput.md)'s 128-count output hysteresis already hides return scatter, and if scatter ever exceeds that the fix is that axis's `hysteresis` argument, not a user-facing control. 


        

<hr>



### variable max 

_Raw value at the axis's high mechanical stop._ 
```C++
uint16_t OpenSkyhawk::AxisCal::max;
```




<hr>



### variable min 

_Raw value at the axis's low mechanical stop._ 
```C++
uint16_t OpenSkyhawk::AxisCal::min;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/AxisCal.h`

