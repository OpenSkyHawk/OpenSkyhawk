

# Class OpenSkyhawk::AnalogMultiPos



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**AnalogMultiPos**](classOpenSkyhawk_1_1AnalogMultiPos.md)



_Resistor-ladder multi-position selector — one analog_ `PinRef` _, a different voltage per position. Emits the resolved position index 0..N-1 over CAN (MULTIPOS dispatch)._[More...](#detailed-description)

* `#include <AnalogMultiPos.h>`



Inherits the following classes: [OpenSkyhawk::MultiPosInput](classOpenSkyhawk_1_1MultiPosInput.md)
































## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**DEFAULT\_DEADBAND**](#variable-default_deadband)   = `1000`<br>_counts trimmed from each band edge_  |
|  constexpr uint16\_t | [**POLL\_MS**](#variable-poll_ms)   = `8`<br>_min interval between ADC reads (ms)_  |


## Public Static Attributes inherited from OpenSkyhawk::MultiPosInput

See [OpenSkyhawk::MultiPosInput](classOpenSkyhawk_1_1MultiPosInput.md)

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**NO\_POSITION**](classOpenSkyhawk_1_1MultiPosInput.md#variable-no_position)   = `0xFFFF`<br>[_**readRaw()**_](classOpenSkyhawk_1_1MultiPosInput.md#function-readraw) _sentinel: "nothing active right now — hold the last position"._ |








































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**AnalogMultiPos**](#function-analogmultipos-12) (uint16\_t controlId, [**PinRef**](classPinRef.md) pin, uint8\_t numPos, const uint16\_t \* posVals, uint16\_t deadband=[**DEFAULT\_DEADBAND**](classOpenSkyhawk_1_1AnalogMultiPos.md#variable-default_deadband)) <br>_Explicit resistor-ladder selector._  |
|   | [**AnalogMultiPos**](#function-analogmultipos-22) (uint16\_t controlId, [**PinRef**](classPinRef.md) pin, uint8\_t numPos, uint16\_t deadband=[**DEFAULT\_DEADBAND**](classOpenSkyhawk_1_1AnalogMultiPos.md#variable-default_deadband)) <br>_Equal-spacing shorthand — positions evenly spaced across the full ADC range._  |
| virtual void | [**configure**](#function-configure) () override<br>_Configure the pin as an input. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._ |
| virtual void | [**forceReport**](#function-forcereport) () override<br>_Force a fresh ADC sample (bypassing the read throttle), then emit the baseline._  |


## Public Functions inherited from OpenSkyhawk::MultiPosInput

See [OpenSkyhawk::MultiPosInput](classOpenSkyhawk_1_1MultiPosInput.md)

| Type | Name |
| ---: | :--- |
| virtual void | [**forceReport**](classOpenSkyhawk_1_1MultiPosInput.md#function-forcereport) () override<br>_Resolve the current position and emit a CAN EVT unconditionally — no debounce._  |
| virtual void | [**poll**](classOpenSkyhawk_1_1MultiPosInput.md#function-poll) () override<br>_Resolve the position, debounce it, emit a CAN EVT on confirmed change._  |
|  uint16\_t | [**position**](classOpenSkyhawk_1_1MultiPosInput.md#function-position) () const<br>_The last confirmed position index (0..N-1)._  |


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
















## Protected Attributes inherited from OpenSkyhawk::MultiPosInput

See [OpenSkyhawk::MultiPosInput](classOpenSkyhawk_1_1MultiPosInput.md)

| Type | Name |
| ---: | :--- |
|  uint16\_t | [**\_controlId**](classOpenSkyhawk_1_1MultiPosInput.md#variable-_controlid)  <br>_DCS/HID control id (routing)._  |
|  uint8\_t | [**\_numPositions**](classOpenSkyhawk_1_1MultiPosInput.md#variable-_numpositions)  <br>_N — number of discrete positions._  |














































## Protected Functions

| Type | Name |
| ---: | :--- |
| virtual uint16\_t | [**readRaw**](#function-readraw) () override<br>_Resolve the instantaneous position index, or NO\_POSITION to hold the last._  |


## Protected Functions inherited from OpenSkyhawk::MultiPosInput

See [OpenSkyhawk::MultiPosInput](classOpenSkyhawk_1_1MultiPosInput.md)

| Type | Name |
| ---: | :--- |
|   | [**MultiPosInput**](classOpenSkyhawk_1_1MultiPosInput.md#function-multiposinput) (uint16\_t controlId, uint8\_t numPositions, uint16\_t debounceMs) <br>_Construct the base._  |
| virtual uint16\_t | [**readRaw**](classOpenSkyhawk_1_1MultiPosInput.md#function-readraw) () = 0<br>_Resolve the instantaneous position index, or NO\_POSITION to hold the last._  |


## Protected Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**InputBase**](classOpenSkyhawk_1_1InputBase.md#function-inputbase) () <br>_Registers this instance into the linked list._  |








## Detailed Description


Subclass of `MultiPosInput` — it shares the debounce / emit-on-change / hold-last / forceReport contract and provides only the analog read: it maps the 16-bit ADC reading to a position via detection bands centred on each position's expected value.


Two constructions:
* **explicit ladder** — pass `posVals[]`, the expected 16-bit ADC value per position. A position whose entry is `ANALOG_NC` has no detent and is never emitted; its neighbours' bands span its place.
* **equal-spacing shorthand** — pass only N; positions are evenly spaced 0..65535.




Detection bands: each position's band reaches half-way to its nearest _valid_ neighbours, minus `deadband` counts on each edge (default 1000). A reading in the deadband gap between two bands resolves to `NO_POSITION`, so the base holds the last position — this gives switch hysteresis, no flicker at a boundary. The ADC is re-read at most every `POLL_MS` (8 ms).


The base debounce window is 0: the deadband gaps provide the filtering, not a timer. 


    
## Public Static Attributes Documentation




### variable DEFAULT\_DEADBAND 

_counts trimmed from each band edge_ 
```C++
constexpr uint16_t OpenSkyhawk::AnalogMultiPos::DEFAULT_DEADBAND;
```




<hr>



### variable POLL\_MS 

_min interval between ADC reads (ms)_ 
```C++
constexpr uint16_t OpenSkyhawk::AnalogMultiPos::POLL_MS;
```




<hr>
## Public Functions Documentation




### function AnalogMultiPos [1/2]

_Explicit resistor-ladder selector._ 
```C++
OpenSkyhawk::AnalogMultiPos::AnalogMultiPos (
    uint16_t controlId,
    PinRef pin,
    uint8_t numPos,
    const uint16_t * posVals,
    uint16_t deadband=DEFAULT_DEADBAND
) 
```





**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing. 
* `pin` analog [**PinRef**](classPinRef.md) (STM32 ADC GPIO or [**ADS1115**](classADS1115.md) channel). 
* `numPos` N — number of positions (valid indices 0..N-1). 
* `posVals` caller-owned array of N expected 16-bit ADC values, one per position. Must outlive this object. Use ANALOG\_NC for a position with no detent. 
* `deadband` counts trimmed from each band edge for hysteresis (default 1000). 




        

<hr>



### function AnalogMultiPos [2/2]

_Equal-spacing shorthand — positions evenly spaced across the full ADC range._ 
```C++
OpenSkyhawk::AnalogMultiPos::AnalogMultiPos (
    uint16_t controlId,
    PinRef pin,
    uint8_t numPos,
    uint16_t deadband=DEFAULT_DEADBAND
) 
```





**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. 
* `pin` analog [**PinRef**](classPinRef.md). 
* `numPos` N — number of positions. 
* `deadband` counts trimmed from each band edge (default 1000). 




        

<hr>



### function configure 

_Configure the pin as an input. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._
```C++
virtual void OpenSkyhawk::AnalogMultiPos::configure () override
```



Implements [*OpenSkyhawk::InputBase::configure*](classOpenSkyhawk_1_1InputBase.md#function-configure)


<hr>



### function forceReport 

_Force a fresh ADC sample (bypassing the read throttle), then emit the baseline._ 
```C++
virtual void OpenSkyhawk::AnalogMultiPos::forceReport () override
```



The boot EVT burst and SYNC\_REQ must report the _current_ position, never a stale cache: at boot — before millis() reaches POLL\_MS — the throttle would otherwise return the uninitialised NO\_POSITION cache and the base would emit position 0; and a SYNC arriving within POLL\_MS of the last poll-read would echo an old reading. Overrides the base. 


        
Implements [*OpenSkyhawk::InputBase::forceReport*](classOpenSkyhawk_1_1InputBase.md#function-forcereport)


<hr>
## Protected Functions Documentation




### function readRaw 

_Resolve the instantaneous position index, or NO\_POSITION to hold the last._ 
```C++
virtual uint16_t OpenSkyhawk::AnalogMultiPos::readRaw () override
```



Implemented per subclass (one-hot pin scan, analog band-resolve, ...). Must be non-blocking. Return a value in 0..numPositions-1, or NO\_POSITION when nothing is currently active.




**Returns:**

resolved index, or NO\_POSITION. 





        
Implements [*OpenSkyhawk::MultiPosInput::readRaw*](classOpenSkyhawk_1_1MultiPosInput.md#function-readraw)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/AnalogMultiPos/AnalogMultiPos.h`

