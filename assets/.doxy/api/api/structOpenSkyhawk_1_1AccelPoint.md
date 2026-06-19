

# Struct OpenSkyhawk::AccelPoint



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**AccelPoint**](structOpenSkyhawk_1_1AccelPoint.md)



_One point on the acceleration curve (SwitecX25 form)._ [More...](#detailed-description)

* `#include <StepperMotor.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint16\_t | [**delayUs**](#variable-delayus)  <br>_inter-step delay at/above that threshold_  |
|  uint16\_t | [**stepThreshold**](#variable-stepthreshold)  <br>_cumulative steps under acceleration_  |












































## Detailed Description


`delayUs` is the inter-step delay once cumulative accel-steps reach `stepThreshold`. The last entry's `delayUs` sets the maximum angular velocity. 


    
## Public Attributes Documentation




### variable delayUs 

_inter-step delay at/above that threshold_ 
```C++
uint16_t OpenSkyhawk::AccelPoint::delayUs;
```




<hr>



### variable stepThreshold 

_cumulative steps under acceleration_ 
```C++
uint16_t OpenSkyhawk::AccelPoint::stepThreshold;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Drivers/StepperMotor/StepperMotor.h`

