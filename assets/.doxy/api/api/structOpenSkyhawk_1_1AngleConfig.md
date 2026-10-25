

# Struct OpenSkyhawk::AngleConfig



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**AngleConfig**](structOpenSkyhawk_1_1AngleConfig.md)



_Tuning for an_ [_**AngleSensorInput**_](classOpenSkyhawk_1_1AngleSensorInput.md) _— the_[_**AnalogConfig**_](structOpenSkyhawk_1_1AnalogConfig.md) _fields that still mean something once the raw window comes from the centre and travel (#295). Set by name:_`{ .hysteresis = 512 }` _. Fields are only ever appended, each with a default._[More...](#detailed-description)

* `#include <AngleSensorInput.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint8\_t | [**ewmaShift**](#variable-ewmashift)   = `[**AnalogConfig::DEFAULT\_EWMA\_SHIFT**](structOpenSkyhawk_1_1AnalogConfig.md#variable-default_ewma_shift)`<br>_EWMA smoothing strength: α = 1/2^ewmaShift (capped to_ [_**AnalogConfig::MAX\_EWMA\_SHIFT**_](structOpenSkyhawk_1_1AnalogConfig.md#variable-max_ewma_shift) _)._ |
|  uint16\_t | [**hysteresis**](#variable-hysteresis)   = `[**AnalogConfig::HYSTERESIS\_AUTO**](structOpenSkyhawk_1_1AnalogConfig.md#variable-hysteresis_auto)`<br> |
|  uint16\_t | [**pollMs**](#variable-pollms)   = `[**AnalogConfig::DEFAULT\_POLL\_MS**](structOpenSkyhawk_1_1AnalogConfig.md#variable-default_poll_ms)`<br> |












































## Detailed Description


There is no reverse here: flip direction with the sensor's DIR pin. 


    
## Public Attributes Documentation




### variable ewmaShift 

_EWMA smoothing strength: α = 1/2^ewmaShift (capped to_ [_**AnalogConfig::MAX\_EWMA\_SHIFT**_](structOpenSkyhawk_1_1AnalogConfig.md#variable-max_ewma_shift) _)._
```C++
uint8_t OpenSkyhawk::AngleConfig::ewmaShift;
```




<hr>



### variable hysteresis 

```C++
uint16_t OpenSkyhawk::AngleConfig::hysteresis;
```



Output counts of movement before a new value is emitted. HYSTERESIS\_AUTO picks by route, like [**AnalogInput**](classOpenSkyhawk_1_1AnalogInput.md): DCS\_HYSTERESIS for DCSIN\_\*, HID\_HYSTERESIS for CTRL\_\*. 


        

<hr>



### variable pollMs 

```C++
uint16_t OpenSkyhawk::AngleConfig::pollMs;
```



Min interval between ADC reads, ms. Per instance; an [**ADS1115**](classADS1115.md) [**PinRef**](classPinRef.md) blocks ~8 ms per conversion regardless. 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/AngleSensorInput/AngleSensorInput.h`

