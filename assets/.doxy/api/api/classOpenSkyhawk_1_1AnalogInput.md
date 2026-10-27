

# Class OpenSkyhawk::AnalogInput



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**AnalogInput**](classOpenSkyhawk_1_1AnalogInput.md)



_Continuous analog input — one analog_ `PinRef` _, normalised to a 16-bit value 0..65535. Emits the smoothed value over CAN (MULTIPOS transport). Self-registers into_[_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._[More...](#detailed-description)

* `#include <AnalogInput.h>`



Inherits the following classes: [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)


Inherited by the following classes: [OpenSkyhawk::AngleSensorInput](classOpenSkyhawk_1_1AngleSensorInput.md)




















































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**AnalogInput**](#function-analoginput) (uint16\_t controlId, [**PinRef**](classPinRef.md) pin, const [**AnalogConfig**](structOpenSkyhawk_1_1AnalogConfig.md) & cfg={}) <br>_Construct a continuous analog input._  |
| virtual void | [**configure**](#function-configure) () override<br>_Configure the pin as an input. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._ |
| virtual void | [**forceReport**](#function-forcereport) () override<br>_Sample fresh (bypassing the throttle) and emit the current value as the baseline._  |
| virtual void | [**poll**](#function-poll) () override<br>_Throttled ADC read + EWMA; emit when the value clears the hysteresis or a rail._  |


## Public Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
| virtual void | [**configure**](classOpenSkyhawk_1_1InputBase.md#function-configure) () <br>_Configure hardware pins for this input._  |
| virtual void | [**forceReport**](classOpenSkyhawk_1_1InputBase.md#function-forcereport) () = 0<br>_Re-assert the control's current absolute state, or emit nothing._  |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**next**](classOpenSkyhawk_1_1InputBase.md#function-next) () const<br>_Next input in the list; nullptr at end._  |
| virtual void | [**poll**](classOpenSkyhawk_1_1InputBase.md#function-poll) () = 0<br>_Read hardware state and emit a CAN EVT if state changed._  |
| virtual void | [**sampleTick**](classOpenSkyhawk_1_1InputBase.md#function-sampletick) () <br>_High-rate sample hook — called from a sampling ISR when the node runs one (e.g._ [_**ShiftBus**_](classOpenSkyhawk_1_1ShiftBus.md) _timer sampling, -DSHIFTBUS\_ISR\_HZ). Default no-op._ |




## Public Static Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**head**](classOpenSkyhawk_1_1InputBase.md#function-head) () <br>_Head of the self-registered linked list._  |










































## Protected Functions

| Type | Name |
| ---: | :--- |
| virtual uint16\_t | [**readRaw**](#function-readraw) () <br>_Read the raw 16-bit source value. The one hook a family member overrides._  |


## Protected Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**InputBase**](classOpenSkyhawk_1_1InputBase.md#function-inputbase) () <br>_Registers this instance into the linked list._  |






## Detailed Description


A **linear** input, not a selector. It shares the MULTIPOS wire transport with the selector family only because DCS-BIOS `set_state` has no separate "continuous" dispatch — the 16-bit value is the control _position_, not an index.


Read path (ports [**DcsBios**](namespaceDcsBios.md) `PotentiometerEWMA`): read the ADC (already 16-bit — STM32 ×16, or [**ADS1115**](classADS1115.md) 0–3.3V → 0–65534), clamp to `[minRaw, maxRaw]`, map to 0..65535 (reverse-aware), then apply an integer EWMA low-pass filter (α = 1/2^`ewmaShift`). A new value is emitted only when the smoothed value moves more than `hysteresis` counts from the last sent value, or when it reaches a rail (0 / 65535) moving toward it — a settled pot is silent, the endpoints always reached.


The ADC is re-read at most every `pollMs` — an [**AnalogConfig**](structOpenSkyhawk_1_1AnalogConfig.md) field, default `DEFAULT_POLL_MS` (8 ms). It is **per instance, not per node**: two AnalogInputs on one board may read at different rates, and [**PanelGroup**](namespacePanelGroup.md) owns no analog timer of its own (it calls `poll()` every iteration and lets each input decide). `forceReport()` samples fresh (bypassing the throttle) and emits the current value as the baseline. Integer EWMA (a shift, not a divide) keeps it cheap on the FPU-less STM32F103.


`pollMs` and `ewmaShift` are **coupled**: the filter time constant is τ ≈ 2^`ewmaShift` × `pollMs` — the defaults give 2^3 × 8 = 64 ms. Halving `pollMs` halves τ unless `ewmaShift` rises to match, so the two are chosen together, never one alone.


[**configure()**](classOpenSkyhawk_1_1AnalogInput.md#function-configure) does not enable internal pull-ups; the wiper drives the pin directly.


Used by: AN/ARC-51A VOL (volume potentiometer). 


    
## Public Functions Documentation




### function AnalogInput 

_Construct a continuous analog input._ 
```C++
OpenSkyhawk::AnalogInput::AnalogInput (
    uint16_t controlId,
    PinRef pin,
    const AnalogConfig & cfg={}
) 
```





**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing — and, with the default config, the hysteresis ([**AnalogConfig::HYSTERESIS\_AUTO**](structOpenSkyhawk_1_1AnalogConfig.md#variable-hysteresis_auto)). 
* `pin` analog [**PinRef**](classPinRef.md) (STM32 ADC GPIO or [**ADS1115**](classADS1115.md) channel). 
* `cfg` tuning; set only the fields that differ, e.g. `{ .reverse = true }`. 




        

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
## Protected Functions Documentation




### function readRaw 

_Read the raw 16-bit source value. The one hook a family member overrides._ 
```C++
virtual uint16_t OpenSkyhawk::AnalogInput::readRaw () 
```



Default: the analog [**PinRef**](classPinRef.md) (or the injected value under ANALOGINPUT\_TEST). A subclass that reinterprets the reading overrides this and calls [**AnalogInput::readRaw()**](classOpenSkyhawk_1_1AnalogInput.md#function-readraw) for the source value — [**AngleSensorInput**](classOpenSkyhawk_1_1AngleSensorInput.md) re-centres it on its centre angle. Everything after this point (clamp, scale, EWMA, hysteresis, emit) is the base's and is shared unchanged. 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/AnalogInput/AnalogInput.h`

