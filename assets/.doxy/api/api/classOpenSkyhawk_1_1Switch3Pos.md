

# Class OpenSkyhawk::Switch3Pos



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**Switch3Pos**](classOpenSkyhawk_1_1Switch3Pos.md)



_Three-position switch (ON-OFF-ON / spring-centred) on two pins. Emits 0 / 1 / 2 over CAN (MULTIPOS dispatch)._ [More...](#detailed-description)

* `#include <Switch3Pos.h>`



Inherits the following classes: [OpenSkyhawk::MultiPosInput](classOpenSkyhawk_1_1MultiPosInput.md)
































## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**DEBOUNCE\_MS**](#variable-debounce_ms)   = `20`<br>_index stability window (ms)._  |


## Public Static Attributes inherited from OpenSkyhawk::MultiPosInput

See [OpenSkyhawk::MultiPosInput](classOpenSkyhawk_1_1MultiPosInput.md)

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**NO\_POSITION**](classOpenSkyhawk_1_1MultiPosInput.md#variable-no_position)   = `0xFFFF`<br>[_**readRaw()**_](classOpenSkyhawk_1_1MultiPosInput.md#function-readraw) _sentinel: "nothing active right now — hold the last position"._ |








































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**Switch3Pos**](#function-switch3pos) (uint16\_t controlId, [**PinRef**](classPinRef.md) pinA, [**PinRef**](classPinRef.md) pinB, bool reverse=false, uint16\_t debounceMs=[**DEBOUNCE\_MS**](classOpenSkyhawk_1_1Switch3Pos.md#variable-debounce_ms)) <br>_Construct a 3-position switch._  |
| virtual void | [**configure**](#function-configure) () override<br>_Configure both pins as inputs. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._ |


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
| virtual uint16\_t | [**readRaw**](#function-readraw) () override<br>_pin A → 0, pin B → 2, neither → 1 (centre). pin A wins if both read active._  |


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


A MULTIPOS-family input (N = 3) — self-registers via [**MultiPosInput**](classOpenSkyhawk_1_1MultiPosInput.md), which owns the debounce / emit-on-change / forceReport contract. This class supplies only the read.


Two pins, one per outer throw; the centre needs no pin of its own:
* pin A active → position 0
* pin B active → position 2
* neither → position 1 (centre — a real, emitted position)




Mirrors [**DcsBios**](namespaceDcsBios.md) `Switch3Pos`: if both pins read active (impossible mechanically — a bounce during a throw), pin A wins (position 0); the debounce absorbs the transient either way. Because the centre is resolved directly, [**readRaw()**](classOpenSkyhawk_1_1Switch3Pos.md#function-readraw) never returns NO\_POSITION — the base's hold-last path is unused.


One-hot read (reverse = false, default): the active pin reads LOW (closed to GND). reverse = true inverts it (active pin reads HIGH). [**configure()**](classOpenSkyhawk_1_1Switch3Pos.md#function-configure) does not enable internal pull-ups; the schematic supplies input bias (typically 10 kΩ to +3.3V, switch to GND).


Used by: AN/ASN-41 LAT/LON slew (spring-centred momentary L / centre / R). 


    
## Public Static Attributes Documentation




### variable DEBOUNCE\_MS 

_index stability window (ms)._ 
```C++
constexpr uint16_t OpenSkyhawk::Switch3Pos::DEBOUNCE_MS;
```




<hr>
## Public Functions Documentation




### function Switch3Pos 

_Construct a 3-position switch._ 
```C++
OpenSkyhawk::Switch3Pos::Switch3Pos (
    uint16_t controlId,
    PinRef pinA,
    PinRef pinB,
    bool reverse=false,
    uint16_t debounceMs=DEBOUNCE_MS
) 
```





**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing. 
* `pinA` outer throw → position 0 (active-LOW unless `reverse`). 
* `pinB` outer throw → position 2. 
* `reverse` false (default): active pin reads LOW. true: active pin reads HIGH. 
* `debounceMs` index stability window before a change is confirmed (default 20 ms). 




        

<hr>



### function configure 

_Configure both pins as inputs. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._
```C++
virtual void OpenSkyhawk::Switch3Pos::configure () override
```



Does not enable internal pull-ups; board wiring supplies input bias.




**Note:**

Must not run from the constructor — MCP23017 register writes require the expander to be initialised first. 





        
Implements [*OpenSkyhawk::InputBase::configure*](classOpenSkyhawk_1_1InputBase.md#function-configure)


<hr>
## Protected Functions Documentation




### function readRaw 

_pin A → 0, pin B → 2, neither → 1 (centre). pin A wins if both read active._ 
```C++
virtual uint16_t OpenSkyhawk::Switch3Pos::readRaw () override
```



Implements [*OpenSkyhawk::MultiPosInput::readRaw*](classOpenSkyhawk_1_1MultiPosInput.md#function-readraw)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/Switch3Pos/Switch3Pos.h`

