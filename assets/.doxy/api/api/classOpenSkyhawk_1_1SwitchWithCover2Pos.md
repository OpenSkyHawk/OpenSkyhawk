

# Class OpenSkyhawk::SwitchWithCover2Pos



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**SwitchWithCover2Pos**](classOpenSkyhawk_1_1SwitchWithCover2Pos.md)



_Guarded switch — the DcsBios::SwitchWithCover2Pos equivalent. One physical switch pin drives two sim controls: the guard cover and the switch it protects._ [More...](#detailed-description)

* `#include <SwitchWithCover2Pos.h>`



Inherits the following classes: [OpenSkyhawk::Switch2Pos](classOpenSkyhawk_1_1Switch2Pos.md)
































## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**COVER\_DELAY\_MS**](#variable-cover_delay_ms)   = `200`<br>_Minimum gap between the cover frame and the switch frame, ms._  |


## Public Static Attributes inherited from OpenSkyhawk::Switch2Pos

See [OpenSkyhawk::Switch2Pos](classOpenSkyhawk_1_1Switch2Pos.md)

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**DEBOUNCE\_MS**](classOpenSkyhawk_1_1Switch2Pos.md#variable-debounce_ms)   = `20`<br> |








































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**SwitchWithCover2Pos**](#function-switchwithcover2pos) (uint16\_t switchId, uint16\_t coverId, [**PinRef**](classPinRef.md) pin, bool reverse=false) <br>_Construct a guarded 2-position switch._  |
| virtual void | [**poll**](#function-poll) () override<br>_Debounce through_ [_**Switch2Pos**_](classOpenSkyhawk_1_1Switch2Pos.md) _, then advance the cover/switch sequence._ |


## Public Functions inherited from OpenSkyhawk::Switch2Pos

See [OpenSkyhawk::Switch2Pos](classOpenSkyhawk_1_1Switch2Pos.md)

| Type | Name |
| ---: | :--- |
|   | [**Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md#function-switch2pos-12) (uint16\_t controlId, [**PinRef**](classPinRef.md) pin) <br>_Construct a 2-position switch with default settings._  |
|   | [**Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md#function-switch2pos-22) (uint16\_t controlId, [**PinRef**](classPinRef.md) pin, bool reverse) <br>_Construct a 2-position switch with explicit polarity._  |
| virtual void | [**configure**](classOpenSkyhawk_1_1Switch2Pos.md#function-configure) () override<br>_Configure the input pin. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _after chip.begin()._ |
| virtual void | [**forceReport**](classOpenSkyhawk_1_1Switch2Pos.md#function-forcereport) () override<br>_Read current pin state and emit EVT unconditionally — no debounce._  |
| virtual void | [**poll**](classOpenSkyhawk_1_1Switch2Pos.md#function-poll) () override<br>_Read current pin state, apply debounce, emit EVT if confirmed state changed._  |


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
















## Protected Attributes inherited from OpenSkyhawk::Switch2Pos

See [OpenSkyhawk::Switch2Pos](classOpenSkyhawk_1_1Switch2Pos.md)

| Type | Name |
| ---: | :--- |
|  uint16\_t | [**\_controlId**](classOpenSkyhawk_1_1Switch2Pos.md#variable-_controlid)  <br> |
|  bool | [**\_lastConfirmed**](classOpenSkyhawk_1_1Switch2Pos.md#variable-_lastconfirmed)  <br> |
|  [**PinRef**](classPinRef.md) | [**\_pin**](classOpenSkyhawk_1_1Switch2Pos.md#variable-_pin)  <br> |
|  bool | [**\_reverse**](classOpenSkyhawk_1_1Switch2Pos.md#variable-_reverse)  <br> |














































## Protected Functions

| Type | Name |
| ---: | :--- |
| virtual void | [**emit**](#function-emit) (bool active, bool init) override<br>_Set the sequence target instead of sending. Called by_ [_**Switch2Pos**_](classOpenSkyhawk_1_1Switch2Pos.md) _on a confirmed change, and with init = true for the boot / SYNC\_REQ baseline._ |


## Protected Functions inherited from OpenSkyhawk::Switch2Pos

See [OpenSkyhawk::Switch2Pos](classOpenSkyhawk_1_1Switch2Pos.md)

| Type | Name |
| ---: | :--- |
| virtual void | [**emit**](classOpenSkyhawk_1_1Switch2Pos.md#function-emit) (bool active, bool init) <br>_Send the debounced state. The one hook a family member overrides._  |


## Protected Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**InputBase**](classOpenSkyhawk_1_1InputBase.md#function-inputbase) () <br>_Registers this instance into the linked list._  |








## Detailed Description


The cover is **not sensed**. There is one switch on the panel, and this class sequences the pair so the sim's cover animates and any sim logic gated on the cover is satisfied:



|Physical switch   |Frames, each COVER\_DELAY\_MS apart   |End state    |
|-----|-----|-----|
|flips **on**   |cover `1` (open) → switch `1`   |cover open, switch on    |
|flips **off**   |switch `0` → cover `0` (close)   |cover closed, switch off   |






States mirror DCS-BIOS — OFF\_CLOSED → OFF\_OPEN → ON\_OPEN and back — and the 20 ms debounce comes from [**Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md). The sequencer steps from [**poll()**](classOpenSkyhawk_1_1SwitchWithCover2Pos.md#function-poll), one frame at a time, and never calls delay(): a node blocking for 200 ms would stall every other control and the CAN drain with it. Flipping back mid-sequence simply retargets the machine, so it walks back the way it came.


On boot and SYNC\_REQ the settled pair is re-asserted in the same order (cover first when on, switch first when off). That is deliberately more than DCS-BIOS does — its reset re-sends nothing — because FirmwarePlan D5 requires every absolute input to be re-sent on sync: a missed input otherwise leaves DCS wrong until the control is next touched.


Does **not** read a cover microswitch: a builder with a sensed cover wires it as its own [**Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md) on the cover's control. Does **not** cover 3-position guarded switches — the DCS-BIOS class is 2-position only, and the A-4E-C's one cover guards a 3-position switch the mod does not gate, so the A-4 build uses [**Switch3Pos**](classOpenSkyhawk_1_1Switch3Pos.md) there. This class is for DCS-BIOS parity and other aircraft. 


    
## Public Static Attributes Documentation




### variable COVER\_DELAY\_MS 

_Minimum gap between the cover frame and the switch frame, ms._ 
```C++
constexpr uint16_t OpenSkyhawk::SwitchWithCover2Pos::COVER_DELAY_MS;
```




<hr>
## Public Functions Documentation




### function SwitchWithCover2Pos 

_Construct a guarded 2-position switch._ 
```C++
OpenSkyhawk::SwitchWithCover2Pos::SwitchWithCover2Pos (
    uint16_t switchId,
    uint16_t coverId,
    PinRef pin,
    bool reverse=false
) 
```





**Parameters:**


* `switchId` DCSIN\_\* constant for the switch under the cover. 
* `coverId` DCSIN\_\* constant for the cover itself (the generator already emits covers as their own control, e.g. AFCS\_1N2\_COVER). 
* `pin` [**PinRef**](classPinRef.md) for the one physical switch (GPIO or MCP23017). 
* `reverse` false (default): active-LOW, as [**Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md). true: active-HIGH. 




        

<hr>



### function poll 

_Debounce through_ [_**Switch2Pos**_](classOpenSkyhawk_1_1Switch2Pos.md) _, then advance the cover/switch sequence._
```C++
virtual void OpenSkyhawk::SwitchWithCover2Pos::poll () override
```



Implements [*OpenSkyhawk::Switch2Pos::poll*](classOpenSkyhawk_1_1Switch2Pos.md#function-poll)


<hr>
## Protected Functions Documentation




### function emit 

_Set the sequence target instead of sending. Called by_ [_**Switch2Pos**_](classOpenSkyhawk_1_1Switch2Pos.md) _on a confirmed change, and with init = true for the boot / SYNC\_REQ baseline._
```C++
virtual void OpenSkyhawk::SwitchWithCover2Pos::emit (
    bool active,
    bool init
) override
```



On init the machine is seeded at the opposite end of the travel so the sequencer re-asserts both frames in the correct order rather than only the one that changed. 


        
Implements [*OpenSkyhawk::Switch2Pos::emit*](classOpenSkyhawk_1_1Switch2Pos.md#function-emit)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/SwitchWithCover2Pos/SwitchWithCover2Pos.h`

