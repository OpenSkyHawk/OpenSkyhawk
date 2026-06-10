

# Class OpenSkyhawk::LED



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**LED**](classOpenSkyhawk_1_1LED.md)



_Drive a GPIO pin from a single bit of a DCS-BIOS output value._ [More...](#detailed-description)

* `#include <PanelGroup.h>`



Inherits the following classes: [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)
























## Public Attributes inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
|  [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) \* | [**next**](classOpenSkyhawk_1_1OutputBase.md#variable-next)  <br>_Next object in the list._  |




## Public Static Attributes inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
|  [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) \* | [**first**](classOpenSkyhawk_1_1OutputBase.md#variable-first)   = `nullptr`<br>_Head of the self-registration linked list._  |


























## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**LED**](#function-led) (uint16\_t addr, uint16\_t mask, uint8\_t pin) <br>_Construct an_ [_**LED**_](classOpenSkyhawk_1_1LED.md) _output object._ |
| virtual void | [**onPacket**](#function-onpacket) (uint16\_t controlId, uint16\_t value) override<br>_Sets pin HIGH when (value & mask) != 0, LOW otherwise._  |


## Public Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md#function-outputbase) () <br>_Registers this object at the head of the linked list._  |
| virtual void | [**onPacket**](classOpenSkyhawk_1_1OutputBase.md#function-onpacket) (uint16\_t controlId, uint16\_t value) = 0<br>_Called by_ [_**PanelGroup::loop()**_](namespacePanelGroup.md#function-loop) _for every received ControlPacket._ |






















































## Detailed Description


Mirrors DcsBios::LED. Pin goes HIGH when (value & mask) is non-zero, LOW otherwise.



```C++
// Direct STM32 GPIO:
OpenSkyhawk::LED warn(A_4E_C_MASTER_CAUTION, 0x4000, PB0);
```
 


    
## Public Functions Documentation




### function LED 

_Construct an_ [_**LED**_](classOpenSkyhawk_1_1LED.md) _output object._
```C++
OpenSkyhawk::LED::LED (
    uint16_t addr,
    uint16_t mask,
    uint8_t pin
) 
```





**Parameters:**


* `addr` DCS-BIOS address for this indicator (used as controlId). 
* `mask` Bitmask to extract the relevant bit from the value word. 
* `pin` GPIO pin to drive (active HIGH). 




        

<hr>



### function onPacket 

_Sets pin HIGH when (value & mask) != 0, LOW otherwise._ 
```C++
virtual void OpenSkyhawk::LED::onPacket (
    uint16_t controlId,
    uint16_t value
) override
```



Implements [*OpenSkyhawk::OutputBase::onPacket*](classOpenSkyhawk_1_1OutputBase.md#function-onpacket)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PanelGroup.h`

