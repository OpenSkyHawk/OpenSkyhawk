

# Class OpenSkyhawk::RotaryEncoder



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**RotaryEncoder**](classOpenSkyhawk_1_1RotaryEncoder.md)



_Incremental quadrature encoder on two pins (A/B). Emits a signed_ **relative** _value per detent over CAN — direction in the sign, magnitude set by the mode. Self-registers into_[_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._[More...](#detailed-description)

* `#include <RotaryEncoder.h>`



Inherits the following classes: [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)


























## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr int16\_t | [**DEFAULT\_STEP**](#variable-default_step)   = `3200`<br>_REL per-detent magnitude (DCS suggested\_step)._  |




























## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**RotaryEncoder**](#function-rotaryencoder) (uint16\_t controlId, [**PinRef**](classPinRef.md) pinA, [**PinRef**](classPinRef.md) pinB, [**EncoderStepsPerDetent**](namespaceOpenSkyhawk.md#enum-encoderstepsperdetent) stepsPerDetent=EncoderStepsPerDetent::One, [**EncoderMode**](namespaceOpenSkyhawk.md#enum-encodermode) mode=EncoderMode::Rel, int16\_t step=[**DEFAULT\_STEP**](classOpenSkyhawk_1_1RotaryEncoder.md#variable-default_step)) <br>_Construct a quadrature encoder._  |
| virtual void | [**configure**](#function-configure) () override<br>_Configure both pins as inputs. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._ |
| virtual void | [**forceReport**](#function-forcereport) () override<br>_Resync the last state; emit nothing (relative control — no baseline)._  |
| virtual void | [**poll**](#function-poll) () override<br>_Decode at loop rate (unless a sampler ticks), then drain pending detents → EVTs._  |
| virtual void | [**sampleTick**](#function-sampletick) () override<br>_ISR-safe quadrature decode of one sample → pending detents. No CAN._  |


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
|  uint8\_t | [**readState**](#function-readstate) () <br>_(pinA &lt;&lt; 1) \| pinB → 0..3._  |


## Protected Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**InputBase**](classOpenSkyhawk_1_1InputBase.md#function-inputbase) () <br>_Registers this instance into the linked list._  |






## Detailed Description


A _relative_ control — it reports motion, not an absolute position. Ports [**DcsBios**](namespaceDcsBios.md) `RotaryEncoder`: each poll reads the 2-bit Gray state `(A<<1)|B`, a transition table accumulates a signed delta, and when `|delta| >= stepsPerDetent` one detent fires (CW positive / CCW negative) and the delta is reduced by one detent. `stepsPerDetent` sets how many quadrature transitions make one emitted click — set it to the encoder's transitions-per-detent so one physical click = one EVT.


Two modes, chosen at construction (see EncoderMode):
* **REL** (variable\_step knob, e.g. ASN-41 nav): emits `±step` on `canIdEvtRel`; the bridge sends `%+d` (e.g. `"+3200"`). `step` is build-side feel (default 3200 ≈ DCS suggested\_step, ~20 detents per full throw); lower it for a finer knob. The magnitude lives here on the node, so retuning needs only a node reflash — never a bridge rebuild.
* **DIR** (fixed\_step selector with no indicator, e.g. ARC-51 freq): emits `±1` on `canIdEvtDir`; the bridge sends `INC`/`DEC`. Stateless — DCS owns the position and clamps at the band edges.




Both modes are preset-safe: [**forceReport()**](classOpenSkyhawk_1_1RotaryEncoder.md#function-forcereport) resyncs the last Gray state and emits **nothing** — a relative control has no baseline to assert at boot / SYNC, so it never clobbers a mission preset. [**configure()**](classOpenSkyhawk_1_1RotaryEncoder.md#function-configure) does not enable internal pull-ups; the schematic biases both pins (external pull-ups; the encoder commons to GND).


**High-rate sampling (#197):** [**sampleTick()**](classOpenSkyhawk_1_1RotaryEncoder.md#function-sampletick) (the generic [**InputBase**](classOpenSkyhawk_1_1InputBase.md) hook) decodes one quadrature sample and accumulates _pending detents_; [**poll()**](classOpenSkyhawk_1_1RotaryEncoder.md#function-poll) drains and emits. When a sampling ISR runs [**sampleTick()**](classOpenSkyhawk_1_1RotaryEncoder.md#function-sampletick) at kHz rate ([**PanelGroup**](namespacePanelGroup.md) wires this — the encoder does not know who samples it or from where), a loop stalled by an OLED flush no longer loses transitions. CAN traffic never originates in [**sampleTick()**](classOpenSkyhawk_1_1RotaryEncoder.md#function-sampletick). Without a sampler, [**poll()**](classOpenSkyhawk_1_1RotaryEncoder.md#function-poll) decodes at loop rate exactly as before. Loop-side API is unchanged in both modes.


Dispatch is sourced from the class (the CAN frame), not the input map — see #147. 


    
## Public Static Attributes Documentation




### variable DEFAULT\_STEP 

_REL per-detent magnitude (DCS suggested\_step)._ 
```C++
constexpr int16_t OpenSkyhawk::RotaryEncoder::DEFAULT_STEP;
```




<hr>
## Public Functions Documentation




### function RotaryEncoder 

_Construct a quadrature encoder._ 
```C++
OpenSkyhawk::RotaryEncoder::RotaryEncoder (
    uint16_t controlId,
    PinRef pinA,
    PinRef pinB,
    EncoderStepsPerDetent stepsPerDetent=EncoderStepsPerDetent::One,
    EncoderMode mode=EncoderMode::Rel,
    int16_t step=DEFAULT_STEP
) 
```





**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing. 
* `pinA` quadrature channel A. 
* `pinB` quadrature channel B (swap A/B to reverse the sensed direction). 
* `stepsPerDetent` quadrature transitions per emitted click (default One; match the encoder). 
* `mode` EncoderMode::Rel (variable\_step, ±step) or Dir (fixed\_step, ±1). Default Rel. 
* `step` REL magnitude emitted per detent (default DEFAULT\_STEP). Ignored in DIR. 




        

<hr>



### function configure 

_Configure both pins as inputs. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._
```C++
virtual void OpenSkyhawk::RotaryEncoder::configure () override
```



Implements [*OpenSkyhawk::InputBase::configure*](classOpenSkyhawk_1_1InputBase.md#function-configure)


<hr>



### function forceReport 

_Resync the last state; emit nothing (relative control — no baseline)._ 
```C++
virtual void OpenSkyhawk::RotaryEncoder::forceReport () override
```



Implements [*OpenSkyhawk::InputBase::forceReport*](classOpenSkyhawk_1_1InputBase.md#function-forcereport)


<hr>



### function poll 

_Decode at loop rate (unless a sampler ticks), then drain pending detents → EVTs._ 
```C++
virtual void OpenSkyhawk::RotaryEncoder::poll () override
```



Implements [*OpenSkyhawk::InputBase::poll*](classOpenSkyhawk_1_1InputBase.md#function-poll)


<hr>



### function sampleTick 

_ISR-safe quadrature decode of one sample → pending detents. No CAN._ 
```C++
virtual void OpenSkyhawk::RotaryEncoder::sampleTick () override
```



Implements [*OpenSkyhawk::InputBase::sampleTick*](classOpenSkyhawk_1_1InputBase.md#function-sampletick)


<hr>
## Protected Functions Documentation




### function readState 

_(pinA &lt;&lt; 1) \| pinB → 0..3._ 
```C++
uint8_t OpenSkyhawk::RotaryEncoder::readState () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/RotaryEncoder/RotaryEncoder.h`

