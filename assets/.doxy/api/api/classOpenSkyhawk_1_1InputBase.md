

# Class OpenSkyhawk::InputBase



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**InputBase**](classOpenSkyhawk_1_1InputBase.md)



_Abstract base for all hardware-polled input objects._ [More...](#detailed-description)

* `#include <PanelGroup.h>`





































## Public Functions

| Type | Name |
| ---: | :--- |
| virtual void | [**configure**](#function-configure) () <br>_Configure hardware pins for this input._  |
| virtual void | [**forceReport**](#function-forcereport) () = 0<br>_Read hardware state and emit a CAN EVT unconditionally._  |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**next**](#function-next) () const<br>_Next input in the list; nullptr at end._  |
| virtual void | [**poll**](#function-poll) () = 0<br>_Read hardware state and emit a CAN EVT if state changed._  |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**head**](#function-head) () <br>_Head of the self-registered linked list._  |






















## Protected Functions

| Type | Name |
| ---: | :--- |
|   | [**InputBase**](#function-inputbase) () <br>_Registers this instance into the linked list._  |




## Detailed Description


Declare concrete input objects at global scope; the constructor self-registers into a static linked list. [**PanelGroup::setup()**](namespacePanelGroup.md#function-setup) calls [**configure()**](classOpenSkyhawk_1_1InputBase.md#function-configure) on every registered input after chip initialisation, then calls [**forceReport()**](classOpenSkyhawk_1_1InputBase.md#function-forcereport) for the boot EVT burst. [**PanelGroup::loop()**](namespacePanelGroup.md#function-loop) calls [**poll()**](classOpenSkyhawk_1_1InputBase.md#function-poll) every iteration. 


    
## Public Functions Documentation




### function configure 

_Configure hardware pins for this input._ 
```C++
inline virtual void OpenSkyhawk::InputBase::configure () 
```



Called by [**PanelGroup::setup()**](namespacePanelGroup.md#function-setup) after chip.init() completes. Call \_pin.configureAsInput() here — not in the constructor — because MCP23017 register writes require chip.init() to have run first.


Default is a no-op. Override in every input class that owns a [**PinRef**](classPinRef.md). 


        

<hr>



### function forceReport 

_Read hardware state and emit a CAN EVT unconditionally._ 
```C++
virtual void OpenSkyhawk::InputBase::forceReport () = 0
```



Called during the boot EVT burst and on every SYNC\_REQ. Bypasses debounce. Establishes the current reading as the new baseline so subsequent [**poll()**](classOpenSkyhawk_1_1InputBase.md#function-poll) calls have a valid comparison point. 


        

<hr>



### function next 

_Next input in the list; nullptr at end._ 
```C++
InputBase * OpenSkyhawk::InputBase::next () const
```




<hr>



### function poll 

_Read hardware state and emit a CAN EVT if state changed._ 
```C++
virtual void OpenSkyhawk::InputBase::poll () = 0
```



Called by [**PanelGroup::loop()**](namespacePanelGroup.md#function-loop) every iteration. Must be non-blocking. Implementations apply their own debounce / filtering and call [**CANProtocol::sendBatched()**](namespaceCANProtocol.md#function-sendbatched) only on confirmed state change. 


        

<hr>
## Public Static Functions Documentation




### function head 

_Head of the self-registered linked list._ 
```C++
static InputBase * OpenSkyhawk::InputBase::head () 
```




<hr>
## Protected Functions Documentation




### function InputBase 

_Registers this instance into the linked list._ 
```C++
OpenSkyhawk::InputBase::InputBase () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PanelGroup.h`

