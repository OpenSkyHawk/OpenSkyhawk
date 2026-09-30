

# Class OpenSkyhawk::Dimmer



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**Dimmer**](classOpenSkyhawk_1_1Dimmer.md)



_PWM dimmer output — the DcsBios::Dimmer equivalent. Maps a 16-bit DCS-BIOS value to PWM duty on a direct GPIO timer pin._ [More...](#detailed-description)

* `#include <Dimmer.h>`



Inherits the following classes: [OpenSkyhawk::AnalogOutput](classOpenSkyhawk_1_1AnalogOutput.md)














## Public Types

| Type | Name |
| ---: | :--- |
| typedef uint16\_t(\*)(uint16\_t value) | [**ScaleFn**](#typedef-scalefn)  <br>_Scale function: raw 16-bit DCS value → 16-bit value handed to writeAnalog()._  |




























































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**Dimmer**](#function-dimmer) (uint16\_t controlId, [**PinRef**](classPinRef.md) pin, [**ScaleFn**](classOpenSkyhawk_1_1Dimmer.md#typedef-scalefn) scale=nullptr) <br>_Construct and register a dimmer._  |
| virtual void | [**configure**](#function-configure) () override<br>_Configure the output pin and drive duty to 0._  |


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
| virtual void | [**apply**](#function-apply) (uint16\_t value) override<br>_Write the value as PWM duty. Called by the base when the decoded value changed._  |


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


Primary use is backlight zone dimming: the low-side MOSFET gate of an [**LED**](classOpenSkyhawk_1_1LED.md) zone, one [**Dimmer**](classOpenSkyhawk_1_1Dimmer.md) per zone (PA6 / PA7 on the [**PanelGroup**](namespacePanelGroup.md) base). The A-4E-C drives five such values — LIGHTS\_CONSOLE, LIGHTS\_INSTRUMENTS, LIGHTS\_FLOOD\_RED, LIGHTS\_FLOOD\_WHITE, APG53A\_GLOW — and any other PWM-proportional load works the same way.


The [**AnalogOutput**](classOpenSkyhawk_1_1AnalogOutput.md) base owns matching, decoding and change dedup; this class is the sink: [**apply()**](classOpenSkyhawk_1_1Dimmer.md#function-apply) writes through [**PinRef::writeAnalog()**](classPinRef.md#function-writeanalog) (16-bit in; the GPIO path outputs value &gt;&gt; 8 as 8-bit duty). An optional scale function reshapes the value first — perceptual dimming curves, inversion.


Takes a [**PinRef**](classPinRef.md) like every control class and checks its type: brightness comes from an STM32 timer channel, so [**configure()**](classOpenSkyhawk_1_1Dimmer.md#function-configure) requires a direct GPIO that has one. Anything else (MCP23017, [**ShiftBus**](classOpenSkyhawk_1_1ShiftBus.md), a GPIO with no timer) is logged and leaves the output disabled rather than degrading silently — writeAnalog() is a no-op through an expander, and analogWrite() on a non-timer GPIO collapses to an on/off threshold. Software PWM through an expander is deliberately not offered: those bits change once per loop or bus transfer, so a duty cycle there would flicker and keep the bus busy, where timer PWM costs no CPU.


On/off threshold behaviour is [**LED**](classOpenSkyhawk_1_1LED.md)'s job, not this class's. Polarity is not invertible — the zone-switch hardware standard is fixed (higher duty = brighter); an inverted drive is a scale function, not a constructor flag.


Duty is driven to 0 during [**configure()**](classOpenSkyhawk_1_1Dimmer.md#function-configure) and stays there (zone dark) until a CTRL\_BCAST packet with a matching controlId arrives. 


    
## Public Types Documentation




### typedef ScaleFn 

_Scale function: raw 16-bit DCS value → 16-bit value handed to writeAnalog()._ 
```C++
using OpenSkyhawk::Dimmer::ScaleFn =  uint16_t (*)(uint16_t value);
```




<hr>
## Public Functions Documentation




### function Dimmer 

_Construct and register a dimmer._ 
```C++
OpenSkyhawk::Dimmer::Dimmer (
    uint16_t controlId,
    PinRef pin,
    ScaleFn scale=nullptr
) 
```





**Parameters:**


* `controlId` DCS-BIOS output address (A\_4E\_C\_\* from [**A4EC\_OutputIds.h**](A4EC__OutputIds_8h.md) — the full 16-bit word; the matching \*\_AM mask is 0xffff for these outputs). 
* `pin` [**PinRef**](classPinRef.md) for the PWM output: a direct STM32 GPIO on a timer channel (the zone-switch MOSFET gate) — [**PinRef(PA6)**](classPinRef.md) / [**PinRef(PA7)**](classPinRef.md) on the base. 
* `scale` Optional reshaping of the 16-bit value before it is written. nullptr (default) = identity. 




        

<hr>



### function configure 

_Configure the output pin and drive duty to 0._ 
```C++
virtual void OpenSkyhawk::Dimmer::configure () override
```



Called by [**PanelGroup::setup()**](namespacePanelGroup.md#function-setup) after chip.init(). Requires a direct GPIO on a timer channel; otherwise logs on DiagSerial and disables the output (no writes at all). On success the pin becomes an output at duty 0, so the zone stays dark until the first matching CTRL\_BCAST. 


        
Implements [*OpenSkyhawk::OutputBase::configure*](classOpenSkyhawk_1_1OutputBase.md#function-configure)


<hr>
## Protected Functions Documentation




### function apply 

_Write the value as PWM duty. Called by the base when the decoded value changed._ 
```C++
virtual void OpenSkyhawk::Dimmer::apply (
    uint16_t value
) override
```



Applies the scale function, then skips the write when the resulting 8-bit duty equals the last one written — different 16-bit values map to the same duty. No-op while the output is disabled. 


        
Implements [*OpenSkyhawk::AnalogOutput::apply*](classOpenSkyhawk_1_1AnalogOutput.md#function-apply)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Outputs/Dimmer/Dimmer.h`

