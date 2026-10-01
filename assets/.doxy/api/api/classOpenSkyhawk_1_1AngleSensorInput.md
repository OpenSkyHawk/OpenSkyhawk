

# Class OpenSkyhawk::AngleSensorInput



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**AngleSensorInput**](classOpenSkyhawk_1_1AngleSensorInput.md)



_Magnetic angle sensor (AS5600 / MT6701) as an absolute knob or axis._ [More...](#detailed-description)

* `#include <AngleSensorInput.h>`



Inherits the following classes: [OpenSkyhawk::AnalogInput](classOpenSkyhawk_1_1AnalogInput.md)
































## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr float | [**COUNTS\_PER\_DEG**](#variable-counts_per_deg)   = `65536.0f / 360.0f`<br>_Counts per degree: a full turn spans the 16-bit range (65536 / 360)._  |


## Public Static Attributes inherited from OpenSkyhawk::AnalogInput

See [OpenSkyhawk::AnalogInput](classOpenSkyhawk_1_1AnalogInput.md)

| Type | Name |
| ---: | :--- |
|  constexpr uint8\_t | [**DEFAULT\_EWMA\_SHIFT**](classOpenSkyhawk_1_1AnalogInput.md#variable-default_ewma_shift)   = `3`<br>_EWMA α = 1/2^3 = 1/8._  |
|  constexpr uint16\_t | [**DEFAULT\_HYSTERESIS**](classOpenSkyhawk_1_1AnalogInput.md#variable-default_hysteresis)   = `128`<br>_counts on the 16-bit output._  |
|  constexpr uint16\_t | [**DEFAULT\_POLL\_MS**](classOpenSkyhawk_1_1AnalogInput.md#variable-default_poll_ms)   = `8`<br>_default min interval between ADC reads (ms)._  |
|  constexpr uint8\_t | [**MAX\_EWMA\_SHIFT**](classOpenSkyhawk_1_1AnalogInput.md#variable-max_ewma_shift)   = `15`<br>_cap: scaled&lt;&lt;shift must fit int32 at full scale._  |








































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**AngleSensorInput**](#function-anglesensorinput) (uint16\_t controlId, [**PinRef**](classPinRef.md) pin, float centerDeg, float travelDeg, uint16\_t pollMs=[**DEFAULT\_POLL\_MS**](classOpenSkyhawk_1_1AnalogInput.md#variable-default_poll_ms)) <br>_Construct an absolute angle input._  |


## Public Functions inherited from OpenSkyhawk::AnalogInput

See [OpenSkyhawk::AnalogInput](classOpenSkyhawk_1_1AnalogInput.md)

| Type | Name |
| ---: | :--- |
|   | [**AnalogInput**](classOpenSkyhawk_1_1AnalogInput.md#function-analoginput) (uint16\_t controlId, [**PinRef**](classPinRef.md) pin, bool reverse=false, uint16\_t minRaw=0, uint16\_t maxRaw=65535, uint16\_t hysteresis=[**DEFAULT\_HYSTERESIS**](classOpenSkyhawk_1_1AnalogInput.md#variable-default_hysteresis), uint8\_t ewmaShift=[**DEFAULT\_EWMA\_SHIFT**](classOpenSkyhawk_1_1AnalogInput.md#variable-default_ewma_shift), uint16\_t pollMs=[**DEFAULT\_POLL\_MS**](classOpenSkyhawk_1_1AnalogInput.md#variable-default_poll_ms)) <br>_Construct a continuous analog input._  |
| virtual void | [**configure**](classOpenSkyhawk_1_1AnalogInput.md#function-configure) () override<br>_Configure the pin as an input. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._ |
| virtual void | [**forceReport**](classOpenSkyhawk_1_1AnalogInput.md#function-forcereport) () override<br>_Sample fresh (bypassing the throttle) and emit the current value as the baseline._  |
| virtual void | [**poll**](classOpenSkyhawk_1_1AnalogInput.md#function-poll) () override<br>_Throttled ADC read + EWMA; emit when the value clears the hysteresis or a rail._  |


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






























































## Protected Functions

| Type | Name |
| ---: | :--- |
| virtual uint16\_t | [**readRaw**](#function-readraw) () override<br>_The base's reading, re-centred so centerDeg sits at mid-scale (0°/360° wraps)._  |


## Protected Functions inherited from OpenSkyhawk::AnalogInput

See [OpenSkyhawk::AnalogInput](classOpenSkyhawk_1_1AnalogInput.md)

| Type | Name |
| ---: | :--- |
| virtual uint16\_t | [**readRaw**](classOpenSkyhawk_1_1AnalogInput.md#function-readraw) () <br>_Read the raw 16-bit source value. The one hook a family member overrides._  |


## Protected Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**InputBase**](classOpenSkyhawk_1_1InputBase.md#function-inputbase) () <br>_Registers this instance into the linked list._  |








## Detailed Description


An `AnalogInput` that reads the sensor's **analog output** through any analog-capable [**PinRef**](classPinRef.md) — an STM32 ADC pin or an [**ADS1115**](classADS1115.md) channel. No wiper to wear out, and a full 360° mechanical range where a pot stops around 270°. Intended for absolute DCS-BIOS knobs such as GUNSIGHT\_KNB and RADAR\_RETICLE; flight-control axes use linear Hall sensors on a plain [**AnalogInput**](classOpenSkyhawk_1_1AnalogInput.md).


The base owns everything that makes an analog input behave: EWMA smoothing, hysteresis, per-instance pollMs, and routing by controlId (DCSIN\_\* → absolute set\_state via [**PanelBridge**](namespacePanelBridge.md); CTRL\_\* → HID via [**SimGateway**](namespaceSimGateway.md)). This class adds only the **angle meaning** — where the travel sits on the circle, and what happens at the 0°/360° seam.


A full turn is 65536 counts, so one degree is 65536/360 ≈ 182 counts. The constructor turns centre and travel into the base's [minRaw, maxRaw] window, and [**readRaw()**](classOpenSkyhawk_1_1AngleSensorInput.md#function-readraw) re-centres every reading so the centre angle lands at mid-scale. Unsigned arithmetic wraps for free, which is what makes a travel spanning 0°/360° (say 340°→40°) behave like any other — no rotating the magnet mount to dodge the seam.


**The seam does not disappear, it moves opposite the centre.** A single-turn absolute sensor reads the same angle at both ends of a full turn, so the circle cannot map onto a line without one break; re-centring puts that break as far from the knob's centre as possible. Travel is therefore up to _just under_ a full turn: at exactly 360° the two ends would be the same sensor angle, and the output would jump full-scale there. A knob that genuinely rotates without end needs different semantics (turn counting, or a relative mode) and is not this class.


Resolution note: the analog output is coarser than the chip's I²C register and picks up ADC noise — fine for a gunsight or reticle knob. The AS5600's ZPOS/MPOS can narrow its output span to the knob's travel to win some of that back. Reading the angle register over I²C belongs in a future [**PinRef**](classPinRef.md) backend, not here, so this class keeps taking a [**PinRef**](classPinRef.md). 


    
## Public Static Attributes Documentation




### variable COUNTS\_PER\_DEG 

_Counts per degree: a full turn spans the 16-bit range (65536 / 360)._ 
```C++
constexpr float OpenSkyhawk::AngleSensorInput::COUNTS_PER_DEG;
```




<hr>
## Public Functions Documentation




### function AngleSensorInput 

_Construct an absolute angle input._ 
```C++
OpenSkyhawk::AngleSensorInput::AngleSensorInput (
    uint16_t controlId,
    PinRef pin,
    float centerDeg,
    float travelDeg,
    uint16_t pollMs=DEFAULT_POLL_MS
) 
```





**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing. 
* `pin` analog [**PinRef**](classPinRef.md) for the sensor's output (STM32 ADC pin or [**ADS1115**](classADS1115.md) channel). 
* `centerDeg` sensor angle at the knob's centre position. Wraps, so 370 == 10. 
* `travelDeg` total mechanical travel, centred on centerDeg: the knob spans centerDeg ± travelDeg/2. Clamped to (0, 360) — up to just under a full turn, because the ends of a full turn are one sensor angle. The wrap point sits opposite centerDeg, so the closer travel comes to 360° the nearer that break sits to the ends of travel. 
* `pollMs` min interval between ADC reads, ms (default DEFAULT\_POLL\_MS). Per instance; an [**ADS1115**](classADS1115.md) [**PinRef**](classPinRef.md) blocks ~8 ms per conversion regardless. 




        

<hr>
## Protected Functions Documentation




### function readRaw 

_The base's reading, re-centred so centerDeg sits at mid-scale (0°/360° wraps)._ 
```C++
virtual uint16_t OpenSkyhawk::AngleSensorInput::readRaw () override
```



Implements [*OpenSkyhawk::AnalogInput::readRaw*](classOpenSkyhawk_1_1AnalogInput.md#function-readraw)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/AngleSensorInput/AngleSensorInput.h`

