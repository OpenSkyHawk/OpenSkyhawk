

# Class OpenSkyhawk::AnalogInput



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**AnalogInput**](classOpenSkyhawk_1_1AnalogInput.md)



_Continuous analog input — one analog_ `PinRef` _, normalised to a 16-bit value 0..65535. Emits the smoothed value over CAN (MULTIPOS transport). Self-registers into_[_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._[More...](#detailed-description)

* `#include <AnalogInput.h>`



Inherits the following classes: [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)


























## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint8\_t | [**DEFAULT\_EWMA\_SHIFT**](#variable-default_ewma_shift)   = `3`<br>_EWMA α = 1/2^3 = 1/8._  |
|  constexpr uint16\_t | [**DEFAULT\_HYSTERESIS**](#variable-default_hysteresis)   = `128`<br>_counts on the 16-bit output._  |
|  constexpr uint8\_t | [**MAX\_EWMA\_SHIFT**](#variable-max_ewma_shift)   = `15`<br>_cap: scaled&lt;&lt;shift must fit int32 at full scale._  |
|  constexpr uint16\_t | [**POLL\_MS**](#variable-poll_ms)   = `8`<br>_min interval between ADC reads (ms)._  |




























## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**AnalogInput**](#function-analoginput) (uint16\_t controlId, [**PinRef**](classPinRef.md) pin, bool reverse=false, uint16\_t minRaw=0, uint16\_t maxRaw=65535, uint16\_t hysteresis=[**DEFAULT\_HYSTERESIS**](classOpenSkyhawk_1_1AnalogInput.md#variable-default_hysteresis), uint8\_t ewmaShift=[**DEFAULT\_EWMA\_SHIFT**](classOpenSkyhawk_1_1AnalogInput.md#variable-default_ewma_shift)) <br>_Construct a continuous analog input._  |
| virtual void | [**configure**](#function-configure) () override<br>_Configure the pin as an input. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._ |
| virtual void | [**forceReport**](#function-forcereport) () override<br>_Sample fresh (bypassing the throttle) and emit the current value as the baseline._  |
| virtual void | [**poll**](#function-poll) () override<br>_Throttled ADC read + EWMA; emit when the value clears the hysteresis or a rail._  |


## Public Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
| virtual void | [**configure**](classOpenSkyhawk_1_1InputBase.md#function-configure) () <br>_Configure hardware pins for this input._  |
| virtual void | [**forceReport**](classOpenSkyhawk_1_1InputBase.md#function-forcereport) () = 0<br>_Read hardware state and emit a CAN EVT unconditionally._  |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**next**](classOpenSkyhawk_1_1InputBase.md#function-next) () const<br>_Next input in the list; nullptr at end._  |
| virtual void | [**poll**](classOpenSkyhawk_1_1InputBase.md#function-poll) () = 0<br>_Read hardware state and emit a CAN EVT if state changed._  |
| virtual void | [**sampleTick**](classOpenSkyhawk_1_1InputBase.md#function-sampletick) () <br>_High-rate sample hook — called from a sampling ISR when the node runs one (e.g._ [_**ShiftBus**_](classOpenSkyhawk_1_1ShiftBus.md) _timer sampling, -DSHIFTBUS\_ISR\_HZ). Default no-op._ |




## Public Static Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**head**](classOpenSkyhawk_1_1InputBase.md#function-head) () <br>_Head of the self-registered linked list._  |












































## Protected Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**InputBase**](classOpenSkyhawk_1_1InputBase.md#function-inputbase) () <br>_Registers this instance into the linked list._  |






## Detailed Description


A **linear** input, not a selector. It shares the MULTIPOS wire transport with the selector family only because DCS-BIOS `set_state` has no separate "continuous" dispatch — the 16-bit value is the control _position_, not an index.


Read path (ports [**DcsBios**](namespaceDcsBios.md) `PotentiometerEWMA`): read the ADC (already 16-bit — STM32 ×16 or [**ADS1115**](classADS1115.md) ×2), clamp to `[minRaw, maxRaw]`, map to 0..65535 (reverse-aware), then apply an integer EWMA low-pass filter (α = 1/2^`ewmaShift`). A new value is emitted only when the smoothed value moves more than `hysteresis` counts from the last sent value, or when it reaches a rail (0 / 65535) moving toward it — so a settled pot is silent and the endpoints are always reached.


The ADC is re-read at most every `POLL_MS` (8 ms); `forceReport()` samples fresh (bypassing the throttle) and emits the current value as the baseline. Integer EWMA (a shift, not a divide) keeps it cheap on the FPU-less STM32F103.


[**configure()**](classOpenSkyhawk_1_1AnalogInput.md#function-configure) does not enable internal pull-ups; the wiper drives the pin directly.


Used by: AN/ARC-51A VOL (volume potentiometer). 


    
## Public Static Attributes Documentation




### variable DEFAULT\_EWMA\_SHIFT 

_EWMA α = 1/2^3 = 1/8._ 
```C++
constexpr uint8_t OpenSkyhawk::AnalogInput::DEFAULT_EWMA_SHIFT;
```




<hr>



### variable DEFAULT\_HYSTERESIS 

_counts on the 16-bit output._ 
```C++
constexpr uint16_t OpenSkyhawk::AnalogInput::DEFAULT_HYSTERESIS;
```




<hr>



### variable MAX\_EWMA\_SHIFT 

_cap: scaled&lt;&lt;shift must fit int32 at full scale._ 
```C++
constexpr uint8_t OpenSkyhawk::AnalogInput::MAX_EWMA_SHIFT;
```




<hr>



### variable POLL\_MS 

_min interval between ADC reads (ms)._ 
```C++
constexpr uint16_t OpenSkyhawk::AnalogInput::POLL_MS;
```




<hr>
## Public Functions Documentation




### function AnalogInput 

_Construct a continuous analog input._ 
```C++
OpenSkyhawk::AnalogInput::AnalogInput (
    uint16_t controlId,
    PinRef pin,
    bool reverse=false,
    uint16_t minRaw=0,
    uint16_t maxRaw=65535,
    uint16_t hysteresis=DEFAULT_HYSTERESIS,
    uint8_t ewmaShift=DEFAULT_EWMA_SHIFT
) 
```





**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing. 
* `pin` analog [**PinRef**](classPinRef.md) (STM32 ADC GPIO or [**ADS1115**](classADS1115.md) channel). 
* `reverse` false (default): minRaw → 0, maxRaw → 65535. true: inverted. 
* `minRaw` raw ADC value mapping to 0 (default 0). Readings below are clamped. 
* `maxRaw` raw ADC value mapping to 65535 (default 65535). Above are clamped. 
* `hysteresis` output counts of movement required before a new value is emitted. 
* `ewmaShift` EWMA smoothing strength: α = 1/2^ewmaShift (default 3 → 1/8). Capped to MAX\_EWMA\_SHIFT (15) — beyond that the int32 accumulator (scaled &lt;&lt; shift) would overflow at full scale. 




        

<hr>



### function configure 

_Configure the pin as an input. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._
```C++
virtual void OpenSkyhawk::AnalogInput::configure () override
```



Implements [*OpenSkyhawk::InputBase::configure*](classOpenSkyhawk_1_1InputBase.md#function-configure)


<hr>



### function forceReport 

_Sample fresh (bypassing the throttle) and emit the current value as the baseline._ 
```C++
virtual void OpenSkyhawk::AnalogInput::forceReport () override
```



Implements [*OpenSkyhawk::InputBase::forceReport*](classOpenSkyhawk_1_1InputBase.md#function-forcereport)


<hr>



### function poll 

_Throttled ADC read + EWMA; emit when the value clears the hysteresis or a rail._ 
```C++
virtual void OpenSkyhawk::AnalogInput::poll () override
```



Implements [*OpenSkyhawk::InputBase::poll*](classOpenSkyhawk_1_1InputBase.md#function-poll)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/AnalogInput/AnalogInput.h`

