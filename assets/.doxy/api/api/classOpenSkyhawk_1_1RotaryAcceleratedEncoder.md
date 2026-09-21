

# Class OpenSkyhawk::RotaryAcceleratedEncoder



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**RotaryAcceleratedEncoder**](classOpenSkyhawk_1_1RotaryAcceleratedEncoder.md)



`RotaryEncoder` _plus the two behaviours of_`DcsBios::RotaryAcceleratedEncoder` _, both on by construction. Header-only: the logic lives in the_[_**RotaryEncoder**_](classOpenSkyhawk_1_1RotaryEncoder.md) _family base._[More...](#detailed-description)

* `#include <RotaryAcceleratedEncoder.h>`



Inherits the following classes: [OpenSkyhawk::RotaryEncoder](classOpenSkyhawk_1_1RotaryEncoder.md)


































## Public Static Attributes inherited from OpenSkyhawk::RotaryEncoder

See [OpenSkyhawk::RotaryEncoder](classOpenSkyhawk_1_1RotaryEncoder.md)

| Type | Name |
| ---: | :--- |
|  constexpr int16\_t | [**DEFAULT\_STEP**](classOpenSkyhawk_1_1RotaryEncoder.md#variable-default_step)   = `3200`<br>_REL per-detent magnitude (DCS suggested\_step)._  |
|  constexpr uint32\_t | [**FAST\_THRESHOLD\_MS**](classOpenSkyhawk_1_1RotaryEncoder.md#variable-fast_threshold_ms)   = `175`<br>_A detent completing sooner than this after the previous one is "fast" (ms). DCS-BIOS value._  |
|  constexpr int8\_t | [**MAX\_MOMENTUM**](classOpenSkyhawk_1_1RotaryEncoder.md#variable-max_momentum)   = `4`<br>_Momentum cap, in detents' worth of transitions (± MAX\_MOMENTUM × stepsPerDetent). DCS-BIOS value._  |
|  constexpr uint32\_t | [**STOPPED\_THRESHOLD\_MS**](classOpenSkyhawk_1_1RotaryEncoder.md#variable-stopped_threshold_ms)   = `500`<br>_With no detent for this long, momentum resets to zero (ms). DCS-BIOS value._  |








































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**RotaryAcceleratedEncoder**](#function-rotaryacceleratedencoder-12) (uint16\_t controlId, [**PinRef**](classPinRef.md) pinA, [**PinRef**](classPinRef.md) pinB, [**EncoderStepsPerDetent**](namespaceOpenSkyhawk.md#enum-encoderstepsperdetent) stepsPerDetent, int16\_t step, int16\_t fastStep) <br>_REL (variable\_step) knob: momentum filter + speed-up._  |
|   | [**RotaryAcceleratedEncoder**](#function-rotaryacceleratedencoder-22) (uint16\_t controlId, [**PinRef**](classPinRef.md) pinA, [**PinRef**](classPinRef.md) pinB, [**EncoderStepsPerDetent**](namespaceOpenSkyhawk.md#enum-encoderstepsperdetent) stepsPerDetent, [**EncoderMode**](namespaceOpenSkyhawk.md#enum-encodermode) mode) <br>_Momentum filter only — for DIR (fixed\_step) selectors._  |


## Public Functions inherited from OpenSkyhawk::RotaryEncoder

See [OpenSkyhawk::RotaryEncoder](classOpenSkyhawk_1_1RotaryEncoder.md)

| Type | Name |
| ---: | :--- |
|   | [**RotaryEncoder**](classOpenSkyhawk_1_1RotaryEncoder.md#function-rotaryencoder-12) (uint16\_t controlId, [**PinRef**](classPinRef.md) pinA, [**PinRef**](classPinRef.md) pinB, [**EncoderStepsPerDetent**](namespaceOpenSkyhawk.md#enum-encoderstepsperdetent) stepsPerDetent=EncoderStepsPerDetent::One, [**EncoderMode**](namespaceOpenSkyhawk.md#enum-encodermode) mode=EncoderMode::Rel, int16\_t step=[**DEFAULT\_STEP**](classOpenSkyhawk_1_1RotaryEncoder.md#variable-default_step)) <br>_Construct a quadrature encoder._  |
| virtual void | [**configure**](classOpenSkyhawk_1_1RotaryEncoder.md#function-configure) () override<br>_Configure both pins as inputs. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._ |
| virtual void | [**forceReport**](classOpenSkyhawk_1_1RotaryEncoder.md#function-forcereport) () override<br>_Resync the last state; emit nothing (relative control — no baseline)._  |
| virtual void | [**poll**](classOpenSkyhawk_1_1RotaryEncoder.md#function-poll) () override<br>_Decode at loop rate (unless a sampler ticks), then drain pending detents → EVTs._  |
| virtual void | [**sampleTick**](classOpenSkyhawk_1_1RotaryEncoder.md#function-sampletick) () override<br>_ISR-safe quadrature decode of one sample → pending detents. No CAN._  |


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
































































## Protected Functions inherited from OpenSkyhawk::RotaryEncoder

See [OpenSkyhawk::RotaryEncoder](classOpenSkyhawk_1_1RotaryEncoder.md)

| Type | Name |
| ---: | :--- |
|   | [**RotaryEncoder**](classOpenSkyhawk_1_1RotaryEncoder.md#function-rotaryencoder-22) (uint16\_t controlId, [**PinRef**](classPinRef.md) pinA, [**PinRef**](classPinRef.md) pinB, [**EncoderStepsPerDetent**](namespaceOpenSkyhawk.md#enum-encoderstepsperdetent) stepsPerDetent, [**EncoderMode**](namespaceOpenSkyhawk.md#enum-encodermode) mode, int16\_t step, bool momentumFilter, int16\_t fastStep) <br>_Family constructor — used by_ [_**RotaryAcceleratedEncoder**_](classOpenSkyhawk_1_1RotaryAcceleratedEncoder.md) _(D16)._ |
|  uint8\_t | [**readState**](classOpenSkyhawk_1_1RotaryEncoder.md#function-readstate) () <br>_(pinA &lt;&lt; 1) \| pinB → 0..3._  |


## Protected Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**InputBase**](classOpenSkyhawk_1_1InputBase.md#function-inputbase) () <br>_Registers this instance into the linked list._  |








## Detailed Description



* **Momentum filter** — the DCS-BIOS class's stated purpose ("noisy/faulty rotaries"). Momentum builds by one per transition in the current direction, up to ±(MAX\_MOMENTUM × stepsPerDetent); a transition against it is dropped and only decays it; after STOPPED\_THRESHOLD\_MS (500 ms) with no detent it resets, so a deliberate reversal from rest registers normally.
* **Speed** (REL only) — a detent completing less than FAST\_THRESHOLD\_MS (175 ms) after the previous one sends `fastStep` instead of `step`. Two-tier, not a ramp. The first detent after a resync is always slow. A fast detent is just a bigger `±step` on the REL frame — no wire, bridge or input-map change (supersedes D9).




DIR mode gets the filter only: the DIR frame carries ±1 and [**PanelBridge**](namespacePanelBridge.md) drops anything else, so there is no speed to add — the way DCS-BIOS users apply the class to INC/DEC controls. 


    
## Public Functions Documentation




### function RotaryAcceleratedEncoder [1/2]

_REL (variable\_step) knob: momentum filter + speed-up._ 
```C++
inline OpenSkyhawk::RotaryAcceleratedEncoder::RotaryAcceleratedEncoder (
    uint16_t controlId,
    PinRef pinA,
    PinRef pinB,
    EncoderStepsPerDetent stepsPerDetent,
    int16_t step,
    int16_t fastStep
) 
```





**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing. 
* `pinA` quadrature channel A. 
* `pinB` quadrature channel B (swap A/B to reverse the sensed direction). 
* `stepsPerDetent` quadrature transitions per emitted click (match the encoder). 
* `step` REL magnitude for a normal detent (DCS suggested\_step is 3200). 
* `fastStep` REL magnitude for a fast detent, same sign as `step` (e.g. 4× step). 0 turns the speed-up off, leaving only the filter. 




        

<hr>



### function RotaryAcceleratedEncoder [2/2]

_Momentum filter only — for DIR (fixed\_step) selectors._ 
```C++
inline OpenSkyhawk::RotaryAcceleratedEncoder::RotaryAcceleratedEncoder (
    uint16_t controlId,
    PinRef pinA,
    PinRef pinB,
    EncoderStepsPerDetent stepsPerDetent,
    EncoderMode mode
) 
```





**Parameters:**


* `mode` EncoderMode::Dir. Passing EncoderMode::Rel here gives a REL knob with the filter and no speed-up (use the other constructor to set `fastStep`). 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/RotaryAcceleratedEncoder/RotaryAcceleratedEncoder.h`

