

# Class OpenSkyhawk::OutputBase



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md)



_Base class for all DCS-driven output objects on a_ [_**PanelGroup**_](namespacePanelGroup.md) _board._[More...](#detailed-description)

* `#include <PanelGroup.h>`





Inherited by the following classes: [OpenSkyhawk::IntegerOutput](classOpenSkyhawk_1_1IntegerOutput.md),  [OpenSkyhawk::LED](classOpenSkyhawk_1_1LED.md)
















## Public Attributes

| Type | Name |
| ---: | :--- |
|  [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) \* | [**next**](#variable-next)  <br>_Next object in the list._  |


## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) \* | [**first**](#variable-first)   = `nullptr`<br>_Head of the self-registration linked list._  |














## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**OutputBase**](#function-outputbase) () <br>_Registers this object at the head of the linked list._  |
| virtual void | [**onPacket**](#function-onpacket) (uint16\_t controlId, uint16\_t value) = 0<br>_Called by_ [_**PanelGroup::loop()**_](namespacePanelGroup.md#function-loop) _for every received ControlPacket._ |




























## Detailed Description


Subclass this to create custom output handlers. Objects declared at global scope self-register into the static linked list; [**PanelGroup::loop()**](namespacePanelGroup.md#function-loop) walks the list on every received CTRL\_BCAST frame. 


    
## Public Attributes Documentation




### variable next 

_Next object in the list._ 
```C++
OutputBase* OpenSkyhawk::OutputBase::next;
```




<hr>
## Public Static Attributes Documentation




### variable first 

_Head of the self-registration linked list._ 
```C++
OpenSkyhawk::OutputBase * OpenSkyhawk::OutputBase::first;
```




<hr>
## Public Functions Documentation




### function OutputBase 

_Registers this object at the head of the linked list._ 
```C++
OpenSkyhawk::OutputBase::OutputBase () 
```




<hr>



### function onPacket 

_Called by_ [_**PanelGroup::loop()**_](namespacePanelGroup.md#function-loop) _for every received ControlPacket._
```C++
virtual void OpenSkyhawk::OutputBase::onPacket (
    uint16_t controlId,
    uint16_t value
) = 0
```



Implementations should check controlId against their own address and update hardware only on a match.




**Parameters:**


* `controlId` controlId field from the received ControlPacket. 
* `value` value field from the received ControlPacket. 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PanelGroup.h`

