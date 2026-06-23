

# Class OpenSkyhawk::RotaryEncoder



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**RotaryEncoder**](classOpenSkyhawk_1_1RotaryEncoder.md)



_Incremental quadrature encoder on two pins (A/B). Emits a_ **direction** _per detent over CAN (ENCODER dispatch): 1 = clockwise, 0 = counter-clockwise. Self-registers into_[_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._[More...](#detailed-description)

* `#include <RotaryEncoder.h>`



Inherits the following classes: [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)






















































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**RotaryEncoder**](#function-rotaryencoder) (uint16\_t controlId, [**PinRef**](classPinRef.md) pinA, [**PinRef**](classPinRef.md) pinB, [**StepsPerDetent**](namespaceOpenSkyhawk.md#enum-stepsperdetent) stepsPerDetent=ONE\_STEP\_PER\_DETENT) <br>_Construct a quadrature encoder._  |
| virtual void | [**configure**](#function-configure) () override<br>_Configure both pins as inputs. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._ |
| virtual void | [**forceReport**](#function-forcereport) () override<br>_Resync the last state; emit nothing (relative control — no baseline)._  |
| virtual void | [**poll**](#function-poll) () override<br>_Read the quadrature state, accumulate, emit a direction once a detent completes._  |


## Public Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
| virtual void | [**configure**](classOpenSkyhawk_1_1InputBase.md#function-configure) () <br>_Configure hardware pins for this input._  |
| virtual void | [**forceReport**](classOpenSkyhawk_1_1InputBase.md#function-forcereport) () = 0<br>_Read hardware state and emit a CAN EVT unconditionally._  |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**next**](classOpenSkyhawk_1_1InputBase.md#function-next) () const<br>_Next input in the list; nullptr at end._  |
| virtual void | [**poll**](classOpenSkyhawk_1_1InputBase.md#function-poll) () = 0<br>_Read hardware state and emit a CAN EVT if state changed._  |




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


A _relative_ control — it reports motion, not an absolute position. Ports [**DcsBios**](namespaceDcsBios.md) `RotaryEncoder`: each poll reads the 2-bit Gray state `(A<<1)|B`, a transition table accumulates a signed delta, and when `|delta| >= stepsPerDetent` a CW (1) or CCW (0) EVT is emitted and the delta is reduced by one detent. `stepsPerDetent` sets how many quadrature transitions make one emitted click — set it to the encoder's transitions-per-detent so one physical click = one EVT.


[**PanelBridge**](namespacePanelBridge.md) maps the 0/1 direction to the control's DCS-BIOS argument strings (`"DEC"`/`"INC"` for fixed\_step, the ± increment for variable\_step) via the input map — so the same class drives both kinds, the difference is entirely in the map entry.


[**forceReport()**](classOpenSkyhawk_1_1RotaryEncoder.md#function-forcereport) resyncs the last state and emits **nothing** — a relative control has no baseline to report at boot / SYNC. [**configure()**](classOpenSkyhawk_1_1RotaryEncoder.md#function-configure) does not enable internal pull-ups; the schematic biases both pins (external pull-ups; the encoder commons to GND).


Used by: AN/ASN-41 ×7 push-to-set knobs (variable\_step), AN/ARC-51A ×4 freq/preset (fixed\_step). 


    
## Public Functions Documentation




### function RotaryEncoder 

_Construct a quadrature encoder._ 
```C++
OpenSkyhawk::RotaryEncoder::RotaryEncoder (
    uint16_t controlId,
    PinRef pinA,
    PinRef pinB,
    StepsPerDetent stepsPerDetent=ONE_STEP_PER_DETENT
) 
```





**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing. 
* `pinA` quadrature channel A. 
* `pinB` quadrature channel B (swap A/B to reverse the sensed direction). 
* `stepsPerDetent` quadrature transitions per emitted click (default ONE). 




        

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

_Read the quadrature state, accumulate, emit a direction once a detent completes._ 
```C++
virtual void OpenSkyhawk::RotaryEncoder::poll () override
```



Implements [*OpenSkyhawk::InputBase::poll*](classOpenSkyhawk_1_1InputBase.md#function-poll)


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

