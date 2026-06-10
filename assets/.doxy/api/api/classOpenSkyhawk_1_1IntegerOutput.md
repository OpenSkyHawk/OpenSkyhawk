

# Class OpenSkyhawk::IntegerOutput



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**IntegerOutput**](classOpenSkyhawk_1_1IntegerOutput.md)



_Call an arbitrary function with the raw value from a ControlPacket._ [More...](#detailed-description)

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
|   | [**IntegerOutput**](#function-integeroutput) (uint16\_t addr, void(\*)(uint16\_t) cb) <br>_Construct an_ [_**IntegerOutput**_](classOpenSkyhawk_1_1IntegerOutput.md) _object._ |
| virtual void | [**onPacket**](#function-onpacket) (uint16\_t controlId, uint16\_t value) override<br>_Calls cb\_(value) if controlId matches addr\_._  |


## Public Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md#function-outputbase) () <br>_Registers this object at the head of the linked list._  |
| virtual void | [**onPacket**](classOpenSkyhawk_1_1OutputBase.md#function-onpacket) (uint16\_t controlId, uint16\_t value) = 0<br>_Called by_ [_**PanelGroup::loop()**_](namespacePanelGroup.md#function-loop) _for every received ControlPacket._ |






















































## Detailed Description


Escape hatch for non-standard output logic — e.g. a value that maps to motor positions, PWM brightness, or multi-bit state machines that do not fit the [**LED**](classOpenSkyhawk_1_1LED.md) pattern. Mirrors DcsBios::IntegerBuffer.



```C++
void onCanopyPos(uint16_t v) { analogWrite(CANOPY_MOTOR, v >> 8); }
OpenSkyhawk::IntegerOutput canopy(A_4E_C_CANOPY_POS, onCanopyPos);
```
 


    
## Public Functions Documentation




### function IntegerOutput 

_Construct an_ [_**IntegerOutput**_](classOpenSkyhawk_1_1IntegerOutput.md) _object._
```C++
OpenSkyhawk::IntegerOutput::IntegerOutput (
    uint16_t addr,
    void(*)(uint16_t) cb
) 
```





**Parameters:**


* `addr` DCS-BIOS address for this control (used as controlId). 
* `cb` Function called with the raw value on every matching packet. 




        

<hr>



### function onPacket 

_Calls cb\_(value) if controlId matches addr\_._ 
```C++
virtual void OpenSkyhawk::IntegerOutput::onPacket (
    uint16_t controlId,
    uint16_t value
) override
```



Implements [*OpenSkyhawk::OutputBase::onPacket*](classOpenSkyhawk_1_1OutputBase.md#function-onpacket)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PanelGroup.h`

