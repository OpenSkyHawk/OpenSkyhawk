

# Class OpenSkyhawk::AnalogOutput



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**AnalogOutput**](classOpenSkyhawk_1_1AnalogOutput.md)



_Abstract base for outputs driven by one 16-bit DCS-BIOS value (FirmwarePlan D16)._ [More...](#detailed-description)

* `#include <AnalogOutput.h>`



Inherits the following classes: [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)


Inherited by the following classes: [OpenSkyhawk::Dimmer](classOpenSkyhawk_1_1Dimmer.md),  [OpenSkyhawk::IntegerOutput](classOpenSkyhawk_1_1IntegerOutput.md)




















































## Public Functions

| Type | Name |
| ---: | :--- |
| virtual void | [**onControlPacket**](#function-oncontrolpacket) (uint16\_t controlId, uint16\_t value) <br>_Match, decode, dedup, then_ [_**apply()**_](classOpenSkyhawk_1_1AnalogOutput.md#function-apply) _. Called by_[_**PanelGroup**_](namespacePanelGroup.md) _for every CTRL\_BCAST packet._ |


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
|   | [**AnalogOutput**](#function-analogoutput) (uint16\_t controlId, uint16\_t mask=0xFFFF, uint8\_t shift=0) <br>_Construct the base. Registration into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s output list is_[_**OutputBase**_](classOpenSkyhawk_1_1OutputBase.md) _'s._ |
| virtual void | [**apply**](#function-apply) (uint16\_t value) = 0<br>_Sink hook — called with the decoded value, only when it changed._  |


## Protected Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md#function-outputbase) () <br>_Registers this instance into the linked list._  |






## Detailed Description


Owns what every member shares, so a subclass is only its sink: controlId matching on each CTRL\_BCAST packet, (value & mask) &gt;&gt; shift decoding — the same address/mask/shift triple DCS-BIOS uses, so packed fields work as well as whole-word outputs — and change dedup, so [**apply()**](classOpenSkyhawk_1_1AnalogOutput.md#function-apply) runs only when the decoded value differs from the last one applied. DCS-BIOS re-broadcasts full state after SYNC\_REQ; without the dedup every subclass would repeat the same work on every resync.


Does not know what the value means or where it goes — that is the subclass's [**apply()**](classOpenSkyhawk_1_1AnalogOutput.md#function-apply). Does not configure pins: a subclass with hardware overrides [**configure()**](classOpenSkyhawk_1_1OutputBase.md#function-configure).


Members: [**Dimmer**](classOpenSkyhawk_1_1Dimmer.md) (PWM duty on a GPIO timer pin), [**IntegerOutput**](classOpenSkyhawk_1_1IntegerOutput.md) (user callback). Needle gauges are deliberately not members — they use [**NeedleGauge**](classOpenSkyhawk_1_1NeedleGauge.md) + a [**MotorDriver**](classOpenSkyhawk_1_1MotorDriver.md), which add calibration and smooth motion. 


    
## Public Functions Documentation




### function onControlPacket 

_Match, decode, dedup, then_ [_**apply()**_](classOpenSkyhawk_1_1AnalogOutput.md#function-apply) _. Called by_[_**PanelGroup**_](namespacePanelGroup.md) _for every CTRL\_BCAST packet._
```C++
virtual void OpenSkyhawk::AnalogOutput::onControlPacket (
    uint16_t controlId,
    uint16_t value
) 
```





**Parameters:**


* `controlId` Incoming packet controlId. Ignored if != the configured one. 
* `value` Raw 16-bit DCS-BIOS value. 




        
Implements [*OpenSkyhawk::OutputBase::onControlPacket*](classOpenSkyhawk_1_1OutputBase.md#function-oncontrolpacket)


<hr>
## Protected Functions Documentation




### function AnalogOutput 

_Construct the base. Registration into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s output list is_[_**OutputBase**_](classOpenSkyhawk_1_1OutputBase.md) _'s._
```C++
OpenSkyhawk::AnalogOutput::AnalogOutput (
    uint16_t controlId,
    uint16_t mask=0xFFFF,
    uint8_t shift=0
) 
```





**Parameters:**


* `controlId` DCS-BIOS output address (A\_4E\_C\_\* from [**A4EC\_OutputIds.h**](A4EC__OutputIds_8h.md)). 
* `mask` Bits of the 16-bit word belonging to this output. Default: all. Use the A\_4E\_C\_\*\_AM constants for packed fields. 
* `shift` Right-shift applied after masking. Default 0. 




        

<hr>



### function apply 

_Sink hook — called with the decoded value, only when it changed._ 
```C++
virtual void OpenSkyhawk::AnalogOutput::apply (
    uint16_t value
) = 0
```





**Parameters:**


* `value` (raw & mask) &gt;&gt; shift. 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Outputs/AnalogOutput/AnalogOutput.h`

