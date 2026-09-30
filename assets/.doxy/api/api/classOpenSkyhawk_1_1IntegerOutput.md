

# Class OpenSkyhawk::IntegerOutput



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**IntegerOutput**](classOpenSkyhawk_1_1IntegerOutput.md)



_Callback output — the DcsBios::IntegerBuffer equivalent, for outputs no built-in class covers (a custom display, an LCD, a bespoke actuator)._ [More...](#detailed-description)

* `#include <IntegerOutput.h>`



Inherits the following classes: [OpenSkyhawk::AnalogOutput](classOpenSkyhawk_1_1AnalogOutput.md)














## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\*)(uint16\_t value) | [**Callback**](#typedef-callback)  <br>_Callback type: receives the decoded value._  |




























































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**IntegerOutput**](#function-integeroutput) (uint16\_t controlId, [**Callback**](classOpenSkyhawk_1_1IntegerOutput.md#typedef-callback) callback, uint16\_t mask=0xFFFF, uint8\_t shift=0) <br>_Construct and register a callback output._  |


## Public Functions inherited from OpenSkyhawk::AnalogOutput

See [OpenSkyhawk::AnalogOutput](classOpenSkyhawk_1_1AnalogOutput.md)

| Type | Name |
| ---: | :--- |
| virtual void | [**onControlPacket**](classOpenSkyhawk_1_1AnalogOutput.md#function-oncontrolpacket) (uint16\_t controlId, uint16\_t value) <br>_Match, decode, dedup, then_ [_**apply()**_](classOpenSkyhawk_1_1AnalogOutput.md#function-apply) _. Called by_[_**PanelGroup**_](namespacePanelGroup.md) _for every CTRL\_BCAST packet._ |


## Public Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
| virtual void | [**configure**](classOpenSkyhawk_1_1OutputBase.md#function-configure) () <br>_Configure hardware pins for this output._  |
|  [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) \* | [**next**](classOpenSkyhawk_1_1OutputBase.md#function-next) () const<br>_Next output in the list; nullptr at end._  |
| virtual void | [**onControlPacket**](classOpenSkyhawk_1_1OutputBase.md#function-oncontrolpacket) (uint16\_t controlId, uint16\_t value) = 0<br>_Called for every non-null ControlPacket in a CTRL\_BCAST frame._  |
| virtual void | [**update**](classOpenSkyhawk_1_1OutputBase.md#function-update) () <br>_Called every_ [_**PanelGroup::loop()**_](namespacePanelGroup.md#function-loop) _iteration._ |






## Public Static Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
|  [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) \* | [**head**](classOpenSkyhawk_1_1OutputBase.md#function-head) () <br>_Head of the self-registered linked list._  |






























































## Protected Functions

| Type | Name |
| ---: | :--- |
| virtual void | [**apply**](#function-apply) (uint16\_t value) override<br>_Invoke the callback with the decoded value. Called by the base on a change._  |


## Protected Functions inherited from OpenSkyhawk::AnalogOutput

See [OpenSkyhawk::AnalogOutput](classOpenSkyhawk_1_1AnalogOutput.md)

| Type | Name |
| ---: | :--- |
|   | [**AnalogOutput**](classOpenSkyhawk_1_1AnalogOutput.md#function-analogoutput) (uint16\_t controlId, uint16\_t mask=0xFFFF, uint8\_t shift=0) <br>_Construct the base. Registration into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s output list is_[_**OutputBase**_](classOpenSkyhawk_1_1OutputBase.md) _'s._ |
| virtual void | [**apply**](classOpenSkyhawk_1_1AnalogOutput.md#function-apply) (uint16\_t value) = 0<br>_Sink hook — called with the decoded value, only when it changed._  |


## Protected Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md#function-outputbase) () <br>_Registers this instance into the linked list._  |








## Detailed Description


Calls a user-supplied callback with the decoded value whenever it changes. No [**PinRef**](classPinRef.md): the callback owns whatever hardware is involved.


Matching, (value & mask) &gt;&gt; shift decoding and change dedup come from the [**AnalogOutput**](classOpenSkyhawk_1_1AnalogOutput.md) base, so the callback runs once per change rather than once per CTRL\_BCAST. It runs in [**PanelGroup::loop()**](namespacePanelGroup.md#function-loop) context — never from an ISR — and must not block: a slow callback delays every other output and the next CAN drain. 


    
## Public Types Documentation




### typedef Callback 

_Callback type: receives the decoded value._ 
```C++
using OpenSkyhawk::IntegerOutput::Callback =  void (*)(uint16_t value);
```




<hr>
## Public Functions Documentation




### function IntegerOutput 

_Construct and register a callback output._ 
```C++
OpenSkyhawk::IntegerOutput::IntegerOutput (
    uint16_t controlId,
    Callback callback,
    uint16_t mask=0xFFFF,
    uint8_t shift=0
) 
```





**Parameters:**


* `controlId` DCS-BIOS output address (A\_4E\_C\_\* from [**A4EC\_OutputIds.h**](A4EC__OutputIds_8h.md)). 
* `callback` Called with the decoded value on every change. nullptr is accepted (the output then does nothing) so a sketch can stub one out. 
* `mask` Bits of the 16-bit word belonging to this output. Default: all. 
* `shift` Right-shift applied after masking. Default 0. 




        

<hr>
## Protected Functions Documentation




### function apply 

_Invoke the callback with the decoded value. Called by the base on a change._ 
```C++
virtual void OpenSkyhawk::IntegerOutput::apply (
    uint16_t value
) override
```



Implements [*OpenSkyhawk::AnalogOutput::apply*](classOpenSkyhawk_1_1AnalogOutput.md#function-apply)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Outputs/IntegerOutput/IntegerOutput.h`

