

# Struct OpenSkyhawk::AnalogConfig



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**AnalogConfig**](structOpenSkyhawk_1_1AnalogConfig.md)



_Tuning for an_ [_**AnalogInput**_](classOpenSkyhawk_1_1AnalogInput.md) _. Every field has a default, so a sketch sets only what differs, by name:_`{ .hysteresis = 32, .ewmaShift = 4, .pollMs = 2 }` _(#295)._[More...](#detailed-description)

* `#include <AnalogInput.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint8\_t | [**ewmaShift**](#variable-ewmashift)   = `[**DEFAULT\_EWMA\_SHIFT**](structOpenSkyhawk_1_1AnalogConfig.md#variable-default_ewma_shift)`<br> |
|  uint16\_t | [**hysteresis**](#variable-hysteresis)   = `[**HYSTERESIS\_AUTO**](structOpenSkyhawk_1_1AnalogConfig.md#variable-hysteresis_auto)`<br>_Output counts of movement required before a new value is emitted (HYSTERESIS\_AUTO: by route)._  |
|  uint16\_t | [**maxRaw**](#variable-maxraw)   = `65535`<br>_Raw ADC value mapping to 65535. Readings above are clamped._  |
|  uint16\_t | [**minRaw**](#variable-minraw)   = `0`<br>_Raw ADC value mapping to 0. Readings below are clamped._  |
|  uint16\_t | [**pollMs**](#variable-pollms)   = `[**DEFAULT\_POLL\_MS**](structOpenSkyhawk_1_1AnalogConfig.md#variable-default_poll_ms)`<br> |
|  bool | [**reverse**](#variable-reverse)   = `false`<br>_false: minRaw → 0, maxRaw → 65535. true: inverted._  |


## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**DCS\_HYSTERESIS**](#variable-dcs_hysteresis)   = `1024`<br>_DCS-BIOS route: 1024 keeps a sweep from flooding DCS-BIOS import (SoakRig and E2E use it)._  |
|  constexpr uint8\_t | [**DEFAULT\_EWMA\_SHIFT**](#variable-default_ewma_shift)   = `3`<br>_EWMA α = 1/2^3 = 1/8._  |
|  constexpr uint16\_t | [**DEFAULT\_POLL\_MS**](#variable-default_poll_ms)   = `8`<br>_default min interval between ADC reads (ms)._  |
|  constexpr uint16\_t | [**HID\_HYSTERESIS**](#variable-hid_hysteresis)   = `128`<br>_HID route: 128 — the value the axis calibration was validated with (#46/#251)._  |
|  constexpr uint16\_t | [**HYSTERESIS\_AUTO**](#variable-hysteresis_auto)   = `0xFFFF`<br> |
|  constexpr uint8\_t | [**MAX\_EWMA\_SHIFT**](#variable-max_ewma_shift)   = `15`<br>_cap: scaled&lt;&lt;shift must fit int32 at full scale._  |










































## Detailed Description


Fields are only ever appended, each with a default, so a new setting never breaks a sketch. The constants are the defaults and limits the fields refer to. 


    
## Public Attributes Documentation




### variable ewmaShift 

```C++
uint8_t OpenSkyhawk::AnalogConfig::ewmaShift;
```



EWMA smoothing strength: α = 1/2^ewmaShift. Capped to MAX\_EWMA\_SHIFT — beyond that the int32 accumulator (scaled &lt;&lt; shift) would overflow at full scale. 


        

<hr>



### variable hysteresis 

_Output counts of movement required before a new value is emitted (HYSTERESIS\_AUTO: by route)._ 
```C++
uint16_t OpenSkyhawk::AnalogConfig::hysteresis;
```




<hr>



### variable maxRaw 

_Raw ADC value mapping to 65535. Readings above are clamped._ 
```C++
uint16_t OpenSkyhawk::AnalogConfig::maxRaw;
```




<hr>



### variable minRaw 

_Raw ADC value mapping to 0. Readings below are clamped._ 
```C++
uint16_t OpenSkyhawk::AnalogConfig::minRaw;
```




<hr>



### variable pollMs 

```C++
uint16_t OpenSkyhawk::AnalogConfig::pollMs;
```



Min interval between ADC reads, ms. Per instance, not per node. Not clamped: 0 means read every loop iteration, which is legal but makes τ track the loop rate — not a controlled quantity. An [**ADS1115**](classADS1115.md) [**PinRef**](classPinRef.md) blocks ~8 ms per conversion whatever this says, so keep those at the default or higher. 


        

<hr>



### variable reverse 

_false: minRaw → 0, maxRaw → 65535. true: inverted._ 
```C++
bool OpenSkyhawk::AnalogConfig::reverse;
```




<hr>
## Public Static Attributes Documentation




### variable DCS\_HYSTERESIS 

_DCS-BIOS route: 1024 keeps a sweep from flooding DCS-BIOS import (SoakRig and E2E use it)._ 
```C++
constexpr uint16_t OpenSkyhawk::AnalogConfig::DCS_HYSTERESIS;
```




<hr>



### variable DEFAULT\_EWMA\_SHIFT 

_EWMA α = 1/2^3 = 1/8._ 
```C++
constexpr uint8_t OpenSkyhawk::AnalogConfig::DEFAULT_EWMA_SHIFT;
```




<hr>



### variable DEFAULT\_POLL\_MS 

_default min interval between ADC reads (ms)._ 
```C++
constexpr uint16_t OpenSkyhawk::AnalogConfig::DEFAULT_POLL_MS;
```




<hr>



### variable HID\_HYSTERESIS 

_HID route: 128 — the value the axis calibration was validated with (#46/#251)._ 
```C++
constexpr uint16_t OpenSkyhawk::AnalogConfig::HID_HYSTERESIS;
```




<hr>



### variable HYSTERESIS\_AUTO 

```C++
constexpr uint16_t OpenSkyhawk::AnalogConfig::HYSTERESIS_AUTO;
```



`hysteresis` default: chosen from the controlId's route when the input is constructed — DCS\_HYSTERESIS for a DCSIN\_\* (DCS-BIOS) id, HID\_HYSTERESIS for a CTRL\_\* (HID) id. Any other value is used as given. 


        

<hr>



### variable MAX\_EWMA\_SHIFT 

_cap: scaled&lt;&lt;shift must fit int32 at full scale._ 
```C++
constexpr uint8_t OpenSkyhawk::AnalogConfig::MAX_EWMA_SHIFT;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/AnalogInput/AnalogInput.h`

