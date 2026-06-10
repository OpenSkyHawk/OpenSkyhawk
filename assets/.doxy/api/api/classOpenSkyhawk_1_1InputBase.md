

# Class OpenSkyhawk::InputBase



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**InputBase**](classOpenSkyhawk_1_1InputBase.md)



_Base class for all hardware-polled input objects on a_ [_**PanelGroup**_](namespacePanelGroup.md) _board._[More...](#detailed-description)

* `#include <PanelGroup.h>`





Inherited by the following classes: [OpenSkyhawk::Switch2Pos](classOpenSkyhawk_1_1Switch2Pos.md)
















## Public Attributes

| Type | Name |
| ---: | :--- |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**next**](#variable-next)  <br>_Next object in the list._  |


## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**first**](#variable-first)   = `nullptr`<br>_Head of the self-registration linked list._  |














## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**InputBase**](#function-inputbase) () <br>_Registers this object at the head of the linked list._  |
| virtual void | [**poll**](#function-poll) () = 0<br>_Read hardware state and call_ [_**PanelGroup::sendEvent()**_](namespacePanelGroup.md#function-sendevent) _on change._ |




























## Detailed Description


Subclass this to create custom input handlers. Objects self-register at construction; [**PanelGroup::loop()**](namespacePanelGroup.md#function-loop) calls [**poll()**](classOpenSkyhawk_1_1InputBase.md#function-poll) on every object each iteration. 


    
## Public Attributes Documentation




### variable next 

_Next object in the list._ 
```C++
InputBase* OpenSkyhawk::InputBase::next;
```




<hr>
## Public Static Attributes Documentation




### variable first 

_Head of the self-registration linked list._ 
```C++
OpenSkyhawk::InputBase * OpenSkyhawk::InputBase::first;
```




<hr>
## Public Functions Documentation




### function InputBase 

_Registers this object at the head of the linked list._ 
```C++
OpenSkyhawk::InputBase::InputBase () 
```




<hr>



### function poll 

_Read hardware state and call_ [_**PanelGroup::sendEvent()**_](namespacePanelGroup.md#function-sendevent) _on change._
```C++
virtual void OpenSkyhawk::InputBase::poll () = 0
```



Called by [**PanelGroup::loop()**](namespacePanelGroup.md#function-loop) every iteration. Implementations must be non-blocking and handle their own debounce. 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PanelGroup.h`

