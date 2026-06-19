

# Struct OpenSkyhawk::GaugeCal



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**GaugeCal**](structOpenSkyhawk_1_1GaugeCal.md)



_Value → position calibration for one gauge._ [More...](#detailed-description)

* `#include <NeedleGauge.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  const uint16\_t \* | [**curveIn**](#variable-curvein)  <br>_ascending DCS breakpoints, or nullptr for linear_  |
|  uint8\_t | [**curveN**](#variable-curven)  <br>_breakpoint count (0 = linear)_  |
|  const uint16\_t \* | [**curveOut**](#variable-curveout)  <br>_matching positions for curveIn_  |
|  int16\_t | [**maxTravel**](#variable-maxtravel)  <br>_motor position at DCS value 65535 (linear path)_  |
|  int16\_t | [**minTravel**](#variable-mintravel)  <br>_motor position at DCS value 0 (linear path)_  |
|  bool | [**reverse**](#variable-reverse)  <br>_flip direction (mounted/wired reversed)_  |












































## Detailed Description


The DCS-BIOS value (0..65535) maps to a motor position. For a linear gauge, 0 → `minTravel` and 65535 → `maxTravel` (either may exceed the other or be negative — a centre-zero gauge sits mid-range). For a non-linear dial (airspeed, VVI), supply a piecewise curve: `curveIn` holds ascending DCS breakpoints and `curveOut` the matching positions (`curveN` entries); intermediate values are linearly interpolated.




**Note:**

`curveOut` is unsigned — non-linear dials use a positive position range. Centre-zero gauges use the linear path (signed `minTravel` / `maxTravel`) instead. 





    
## Public Attributes Documentation




### variable curveIn 

_ascending DCS breakpoints, or nullptr for linear_ 
```C++
const uint16_t* OpenSkyhawk::GaugeCal::curveIn;
```




<hr>



### variable curveN 

_breakpoint count (0 = linear)_ 
```C++
uint8_t OpenSkyhawk::GaugeCal::curveN;
```




<hr>



### variable curveOut 

_matching positions for curveIn_ 
```C++
const uint16_t* OpenSkyhawk::GaugeCal::curveOut;
```




<hr>



### variable maxTravel 

_motor position at DCS value 65535 (linear path)_ 
```C++
int16_t OpenSkyhawk::GaugeCal::maxTravel;
```




<hr>



### variable minTravel 

_motor position at DCS value 0 (linear path)_ 
```C++
int16_t OpenSkyhawk::GaugeCal::minTravel;
```




<hr>



### variable reverse 

_flip direction (mounted/wired reversed)_ 
```C++
bool OpenSkyhawk::GaugeCal::reverse;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Outputs/NeedleGauge/NeedleGauge.h`

