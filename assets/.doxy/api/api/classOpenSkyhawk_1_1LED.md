

# Class OpenSkyhawk::LED



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**LED**](classOpenSkyhawk_1_1LED.md)



_Digital_ [_**LED**_](classOpenSkyhawk_1_1LED.md) _output. Drives a pin based on a DCS-BIOS state value._[More...](#detailed-description)

* `#include <LED.h>`



Inherits the following classes: [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)






















































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**LED**](#function-led) (uint16\_t controlId, uint16\_t mask, [**PinRef**](classPinRef.md) pin, bool reverse=false) <br>_Construct and register an_ [_**LED**_](classOpenSkyhawk_1_1LED.md) _output._ |
| virtual void | [**configure**](#function-configure) () override<br>_Configure pin as output and drive it to the off state._  |
| virtual void | [**onControlPacket**](#function-oncontrolpacket) (uint16\_t controlId, uint16\_t value) override<br>_Update_ [_**LED**_](classOpenSkyhawk_1_1LED.md) _state from a CTRL\_BCAST ControlPacket._ |


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












































## Protected Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md#function-outputbase) () <br>_Registers this instance into the linked list._  |






## Detailed Description


Receives state via [**onControlPacket()**](classOpenSkyhawk_1_1LED.md#function-oncontrolpacket) — called by [**PanelGroup**](namespacePanelGroup.md) when a CTRL\_BCAST frame arrives. Ignores packets whose controlId does not match.


The mask parameter handles DCS-BIOS bit-packed outputs, where a single 16-bit address carries multiple independent flags. For whole-word binary outputs (value is 0 or 1), use mask = 0xFFFF.


The reverse parameter handles LEDs wired with current-sinking polarity — for example, an indicator [**LED**](classOpenSkyhawk_1_1LED.md) with its anode tied to VCC through a resistor, where the MCU or MCP23017 output sinks current (LOW = on). reverse = false (default): (value & mask) != 0 → pin HIGH (on). reverse = true: (value & mask) != 0 → pin LOW (on).


Pin is driven to the off state during [**configure()**](classOpenSkyhawk_1_1LED.md#function-configure) and remains off until a CTRL\_BCAST packet with a matching controlId is received.


Works with GPIO and MCP23017 PinRefs. [**ADS1115**](classADS1115.md) PinRefs are input-only — do not assign one to an [**LED**](classOpenSkyhawk_1_1LED.md). 


    
## Public Functions Documentation




### function LED 

_Construct and register an_ [_**LED**_](classOpenSkyhawk_1_1LED.md) _output._
```C++
OpenSkyhawk::LED::LED (
    uint16_t controlId,
    uint16_t mask,
    PinRef pin,
    bool reverse=false
) 
```





**Parameters:**


* `controlId` DCS-BIOS output address (A\_4E\_C\_\* from [**A4EC\_OutputIds.h**](A4EC__OutputIds_8h.md)). 
* `mask` Bitmask: (value & mask) != 0 → on; == 0 → off. Use A\_4E\_C\_\*\_AM constants, or 0xFFFF for whole-word outputs. 
* `pin` GPIO or MCP23017 [**PinRef**](classPinRef.md) for the [**LED**](classOpenSkyhawk_1_1LED.md) pin. 
* `reverse` false (default): pin HIGH = on (current-source wiring). true: pin LOW = on (current-sink, anode to VCC). 




        

<hr>



### function configure 

_Configure pin as output and drive it to the off state._ 
```C++
virtual void OpenSkyhawk::LED::configure () override
```



Called by [**PanelGroup::setup()**](namespacePanelGroup.md#function-setup) after chip.init(). Sets OUTPUT mode on GPIO pins; sets IODIR=0 and GPPU=0 on MCP23017 pins. Drives off immediately: LOW when reverse = false, HIGH when reverse = true. 


        
Implements [*OpenSkyhawk::OutputBase::configure*](classOpenSkyhawk_1_1OutputBase.md#function-configure)


<hr>



### function onControlPacket 

_Update_ [_**LED**_](classOpenSkyhawk_1_1LED.md) _state from a CTRL\_BCAST ControlPacket._
```C++
virtual void OpenSkyhawk::LED::onControlPacket (
    uint16_t controlId,
    uint16_t value
) override
```





**Parameters:**


* `controlId` Incoming packet controlId. Ignored if != \_controlId. 
* `value` Raw 16-bit DCS-BIOS value. 




        
Implements [*OpenSkyhawk::OutputBase::onControlPacket*](classOpenSkyhawk_1_1OutputBase.md#function-oncontrolpacket)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/LED.h`

