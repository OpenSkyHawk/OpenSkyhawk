

# Class OpenSkyhawk::OutputBase



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md)



_Abstract base for all DCS-driven output objects._ [More...](#detailed-description)

* `#include <PanelGroup.h>`





Inherited by the following classes: [OpenSkyhawk::LED](classOpenSkyhawk_1_1LED.md)
































## Public Functions

| Type | Name |
| ---: | :--- |
| virtual void | [**configure**](#function-configure) () <br>_Configure hardware pins for this output._  |
|  [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) \* | [**next**](#function-next) () const<br>_Next output in the list; nullptr at end._  |
| virtual void | [**onControlPacket**](#function-oncontrolpacket) (uint16\_t controlId, uint16\_t value) = 0<br>_Called for every non-null ControlPacket in a CTRL\_BCAST frame._  |
| virtual void | [**update**](#function-update) () <br>_Called every_ [_**PanelGroup::loop()**_](namespacePanelGroup.md#function-loop) _iteration._ |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) \* | [**head**](#function-head) () <br>_Head of the self-registered linked list._  |






















## Protected Functions

| Type | Name |
| ---: | :--- |
|   | [**OutputBase**](#function-outputbase) () <br>_Registers this instance into the linked list._  |




## Detailed Description


Declare concrete output objects at global scope; the constructor self-registers into a static linked list. [**PanelGroup::setup()**](namespacePanelGroup.md#function-setup) calls [**configure()**](classOpenSkyhawk_1_1OutputBase.md#function-configure) after chip initialisation. [**PanelGroup::loop()**](namespacePanelGroup.md#function-loop) calls [**onControlPacket()**](classOpenSkyhawk_1_1OutputBase.md#function-oncontrolpacket) for every non-null slot in each received CTRL\_BCAST frame and calls [**update()**](classOpenSkyhawk_1_1OutputBase.md#function-update) every iteration. 


    
## Public Functions Documentation




### function configure 

_Configure hardware pins for this output._ 
```C++
inline virtual void OpenSkyhawk::OutputBase::configure () 
```



Called by [**PanelGroup::setup()**](namespacePanelGroup.md#function-setup) after chip.init() completes. Call \_pin.configureAsOutput() here. Default is a no-op. 


        

<hr>



### function next 

_Next output in the list; nullptr at end._ 
```C++
OutputBase * OpenSkyhawk::OutputBase::next () const
```




<hr>



### function onControlPacket 

_Called for every non-null ControlPacket in a CTRL\_BCAST frame._ 
```C++
virtual void OpenSkyhawk::OutputBase::onControlPacket (
    uint16_t controlId,
    uint16_t value
) = 0
```





**Parameters:**


* `controlId` DCS-BIOS output address from the received packet. 
* `value` 16-bit value from DCS-BIOS. 




        

<hr>



### function update 

_Called every_ [_**PanelGroup::loop()**_](namespacePanelGroup.md#function-loop) _iteration._
```C++
inline virtual void OpenSkyhawk::OutputBase::update () 
```



Default is a no-op. Override for outputs that need continuous work between CAN frames (stepper motor step generation, PWM updates). 


        

<hr>
## Public Static Functions Documentation




### function head 

_Head of the self-registered linked list._ 
```C++
static OutputBase * OpenSkyhawk::OutputBase::head () 
```




<hr>
## Protected Functions Documentation




### function OutputBase 

_Registers this instance into the linked list._ 
```C++
OpenSkyhawk::OutputBase::OutputBase () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PanelGroup.h`

