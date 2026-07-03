

# Class OpenSkyhawk::SwitchMultiPos



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**SwitchMultiPos**](classOpenSkyhawk_1_1SwitchMultiPos.md)



_Multi-position rotary selector — N discrete pins, exactly one active at a time. Emits the active position index 0..N-1 over CAN (MULTIPOS dispatch)._ [More...](#detailed-description)

* `#include <SwitchMultiPos.h>`



Inherits the following classes: [OpenSkyhawk::MultiPosInput](classOpenSkyhawk_1_1MultiPosInput.md)
































## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**DEBOUNCE\_MS**](#variable-debounce_ms)   = `20`<br> |


## Public Static Attributes inherited from OpenSkyhawk::MultiPosInput

See [OpenSkyhawk::MultiPosInput](classOpenSkyhawk_1_1MultiPosInput.md)

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**NO\_POSITION**](classOpenSkyhawk_1_1MultiPosInput.md#variable-no_position)   = `0xFFFF`<br>[_**readRaw()**_](classOpenSkyhawk_1_1MultiPosInput.md#function-readraw) _sentinel: "nothing active right now — hold the last position"._ |








































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**SwitchMultiPos**](#function-switchmultipos) (uint16\_t controlId, const [**PinRef**](classPinRef.md) \* pins, uint8\_t numPins, bool reverse=false) <br>_Construct an N-position selector._  |
| virtual void | [**configure**](#function-configure) () override<br>_Configure each non-NC pin as an input. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._ |


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
| virtual void | [**sampleTick**](classOpenSkyhawk_1_1InputBase.md#function-sampletick) () <br>_High-rate sample hook — called from a sampling ISR when the node runs one (e.g._ [_**ShiftBus**_](classOpenSkyhawk_1_1ShiftBus.md) _timer sampling, -DSHIFTBUS\_ISR\_HZ). Default no-op._ |






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
| virtual uint16\_t | [**readRaw**](#function-readraw) () override<br>_One-hot scan: return the index of the first active pin, or the PIN\_NC detent index, or NO\_POSITION if nothing is active._  |


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


Self-registers into [**PanelGroup**](namespacePanelGroup.md)'s [**InputBase**](classOpenSkyhawk_1_1InputBase.md) list (via [**MultiPosInput**](classOpenSkyhawk_1_1MultiPosInput.md)).


One-hot read (reverse = false, default): the active pin reads LOW (closed to GND); its array index is the position. reverse = true inverts it (active pin reads HIGH).


If no pin reads active — e.g. a non-shorting rotary mid-throw — the last confirmed position is held; no spurious EVT. A `PIN_NC` entry marks a mechanical-only detent (a position with no physical pin, such as a sprung OFF): when no electrical pin is active, that detent's index is reported.


Debounce: 20 ms on the resolved index — a changed position must hold steady for the window before it is confirmed and emitted. Absorbs contact bounce and fast throws (intermediate detents held &lt; 20 ms are skipped; only the settled position emits). The value is an absolute index, so jumping across positions is safe.


[**configure()**](classOpenSkyhawk_1_1SwitchMultiPos.md#function-configure) does not enable internal pull-ups; the schematic supplies input bias (typically 10 kΩ to +3.3V, switch to GND). 


    
## Public Static Attributes Documentation




### variable DEBOUNCE\_MS 

```C++
constexpr uint16_t OpenSkyhawk::SwitchMultiPos::DEBOUNCE_MS;
```




<hr>
## Public Functions Documentation




### function SwitchMultiPos 

_Construct an N-position selector._ 
```C++
OpenSkyhawk::SwitchMultiPos::SwitchMultiPos (
    uint16_t controlId,
    const PinRef * pins,
    uint8_t numPins,
    bool reverse=false
) 
```





**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing. 
* `pins` Pointer to a caller-owned array of N PinRefs, one per position. The array must outlive this object (define it static/global, like the sketch wiring map). Use PIN\_NC for a mechanical-only detent. 
* `numPins` N — number of entries in `pins` (valid indices 0..N-1). 
* `reverse` false (default): active pin reads LOW. true: active pin reads HIGH. 




        

<hr>



### function configure 

_Configure each non-NC pin as an input. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._
```C++
virtual void OpenSkyhawk::SwitchMultiPos::configure () override
```



Does not enable internal pull-ups; board wiring supplies input bias.




**Note:**

Must not run from the constructor — MCP23017 register writes require the expander to be initialised first. 





        
Implements [*OpenSkyhawk::InputBase::configure*](classOpenSkyhawk_1_1InputBase.md#function-configure)


<hr>
## Protected Functions Documentation




### function readRaw 

_One-hot scan: return the index of the first active pin, or the PIN\_NC detent index, or NO\_POSITION if nothing is active._ 
```C++
virtual uint16_t OpenSkyhawk::SwitchMultiPos::readRaw () override
```



Implements [*OpenSkyhawk::MultiPosInput::readRaw*](classOpenSkyhawk_1_1MultiPosInput.md#function-readraw)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/SwitchMultiPos/SwitchMultiPos.h`

