

# Class OpenSkyhawk::MultiPosInput



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**MultiPosInput**](classOpenSkyhawk_1_1MultiPosInput.md)



_Base for the MULTIPOS input family — selectors that emit an absolute position index 0..N-1 over CAN. Self-registers into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._[More...](#detailed-description)

* `#include <MultiPosInput.h>`



Inherits the following classes: [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)


Inherited by the following classes: [OpenSkyhawk::AnalogMultiPos](classOpenSkyhawk_1_1AnalogMultiPos.md),  [OpenSkyhawk::Switch3Pos](classOpenSkyhawk_1_1Switch3Pos.md),  [OpenSkyhawk::SwitchMultiPos](classOpenSkyhawk_1_1SwitchMultiPos.md)
























## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**NO\_POSITION**](#variable-no_position)   = `0xFFFF`<br>[_**readRaw()**_](classOpenSkyhawk_1_1MultiPosInput.md#function-readraw) _sentinel: "nothing active right now — hold the last position"._ |




























## Public Functions

| Type | Name |
| ---: | :--- |
| virtual void | [**forceReport**](#function-forcereport) () override<br>_Resolve the current position and emit a CAN EVT unconditionally — no debounce._  |
| virtual void | [**poll**](#function-poll) () override<br>_Resolve the position, debounce it, emit a CAN EVT on confirmed change._  |
|  uint16\_t | [**position**](#function-position) () const<br>_The last confirmed position index (0..N-1)._  |


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










## Protected Attributes

| Type | Name |
| ---: | :--- |
|  uint16\_t | [**\_controlId**](#variable-_controlid)  <br>_DCS/HID control id (routing)._  |
|  uint8\_t | [**\_numPositions**](#variable-_numpositions)  <br>_N — number of discrete positions._  |
































## Protected Functions

| Type | Name |
| ---: | :--- |
|   | [**MultiPosInput**](#function-multiposinput) (uint16\_t controlId, uint8\_t numPositions, uint16\_t debounceMs) <br>_Construct the base._  |
| virtual uint16\_t | [**readRaw**](#function-readraw) () = 0<br>_Resolve the instantaneous position index, or NO\_POSITION to hold the last._  |


## Protected Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**InputBase**](classOpenSkyhawk_1_1InputBase.md#function-inputbase) () <br>_Registers this instance into the linked list._  |






## Detailed Description


A subclass reports the instantaneous resolved position via [**readRaw()**](classOpenSkyhawk_1_1MultiPosInput.md#function-readraw); this base debounces it (configurable window) and emits a CAN EVT only when the _confirmed_ position changes. The emitted value is the absolute index, never a delta — a jump from any position to any other (even skipping intermediates) emits the new index directly; there is no ±1 or adjacency assumption.


[**readRaw()**](classOpenSkyhawk_1_1MultiPosInput.md#function-readraw) returns NO\_POSITION when nothing resolves (e.g. a non-shorting rotary mid-throw with no pin closed); the base then holds the last confirmed position — no spurious EVT.


Subclasses: [**SwitchMultiPos**](classOpenSkyhawk_1_1SwitchMultiPos.md) (one-hot pins), [**AnalogMultiPos**](classOpenSkyhawk_1_1AnalogMultiPos.md) (resistor ladder, #114). 


    
## Public Static Attributes Documentation




### variable NO\_POSITION 

[_**readRaw()**_](classOpenSkyhawk_1_1MultiPosInput.md#function-readraw) _sentinel: "nothing active right now — hold the last position"._
```C++
constexpr uint16_t OpenSkyhawk::MultiPosInput::NO_POSITION;
```




<hr>
## Public Functions Documentation




### function forceReport 

_Resolve the current position and emit a CAN EVT unconditionally — no debounce._ 
```C++
virtual void OpenSkyhawk::MultiPosInput::forceReport () override
```



Called by [**PanelGroup**](namespacePanelGroup.md) during the boot EVT burst and on SYNC\_REQ. Establishes the baseline so subsequent [**poll()**](classOpenSkyhawk_1_1MultiPosInput.md#function-poll) calls have a valid reference. 


        
Implements [*OpenSkyhawk::InputBase::forceReport*](classOpenSkyhawk_1_1InputBase.md#function-forcereport)


<hr>



### function poll 

_Resolve the position, debounce it, emit a CAN EVT on confirmed change._ 
```C++
virtual void OpenSkyhawk::MultiPosInput::poll () override
```



Called by [**PanelGroup::loop()**](namespacePanelGroup.md#function-loop). No-op until [**forceReport()**](classOpenSkyhawk_1_1MultiPosInput.md#function-forcereport) has run once. 


        
Implements [*OpenSkyhawk::InputBase::poll*](classOpenSkyhawk_1_1InputBase.md#function-poll)


<hr>



### function position 

_The last confirmed position index (0..N-1)._ 
```C++
inline uint16_t OpenSkyhawk::MultiPosInput::position () const
```



Updated by [**forceReport()**](classOpenSkyhawk_1_1MultiPosInput.md#function-forcereport) (immediately) and [**poll()**](classOpenSkyhawk_1_1MultiPosInput.md#function-poll) (after the debounce confirms a change). Useful as a query — a node can read its current selector position — and for tests to assert the resolved index without capturing the CAN frame. 


        

<hr>
## Protected Attributes Documentation




### variable \_controlId 

_DCS/HID control id (routing)._ 
```C++
uint16_t OpenSkyhawk::MultiPosInput::_controlId;
```




<hr>



### variable \_numPositions 

_N — number of discrete positions._ 
```C++
uint8_t OpenSkyhawk::MultiPosInput::_numPositions;
```




<hr>
## Protected Functions Documentation




### function MultiPosInput 

_Construct the base._ 
```C++
OpenSkyhawk::MultiPosInput::MultiPosInput (
    uint16_t controlId,
    uint8_t numPositions,
    uint16_t debounceMs
) 
```





**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing. 
* `numPositions` number of discrete positions N (valid indices 0..N-1). 
* `debounceMs` stability window before a changed position is confirmed. 0 = confirm on the next poll (subclass does its own filtering). 




        

<hr>



### function readRaw 

_Resolve the instantaneous position index, or NO\_POSITION to hold the last._ 
```C++
virtual uint16_t OpenSkyhawk::MultiPosInput::readRaw () = 0
```



Implemented per subclass (one-hot pin scan, analog band-resolve, ...). Must be non-blocking. Return a value in 0..numPositions-1, or NO\_POSITION when nothing is currently active.




**Returns:**

resolved index, or NO\_POSITION. 





        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/MultiPosInput/MultiPosInput.h`

