

# Class OpenSkyhawk::Switch2Pos



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md)



_Debounced 2-position GPIO switch — sends a ControlPacket CAN event on change._ [More...](#detailed-description)

* `#include <PanelGroup.h>`



Inherits the following classes: [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)
























## Public Attributes inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**next**](classOpenSkyhawk_1_1InputBase.md#variable-next)  <br>_Next object in the list._  |




## Public Static Attributes inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**first**](classOpenSkyhawk_1_1InputBase.md#variable-first)   = `nullptr`<br>_Head of the self-registration linked list._  |


























## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**Switch2Pos**](#function-switch2pos) (uint16\_t addr, uint8\_t pin) <br>_Construct a_ [_**Switch2Pos**_](classOpenSkyhawk_1_1Switch2Pos.md) _input object._ |
| virtual void | [**poll**](#function-poll) () override<br>_Read pin, debounce, and call_ [_**PanelGroup::sendEvent()**_](namespacePanelGroup.md#function-sendevent) _on state change._ |


## Public Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**InputBase**](classOpenSkyhawk_1_1InputBase.md#function-inputbase) () <br>_Registers this object at the head of the linked list._  |
| virtual void | [**poll**](classOpenSkyhawk_1_1InputBase.md#function-poll) () = 0<br>_Read hardware state and call_ [_**PanelGroup::sendEvent()**_](namespacePanelGroup.md#function-sendevent) _on change._ |






















































## Detailed Description


Mirrors DcsBios::Switch2Pos but sends a CAN packet via [**PanelGroup::sendEvent()**](namespacePanelGroup.md#function-sendevent) instead of writing to DCS-BIOS directly. The matching OpenSkyhawk::DCSInput on the [**SimGateway**](namespaceSimGateway.md) translates the packet to a DCS-BIOS message.


Value sent: 1 when pin is LOW (switch closed to GND), 0 when HIGH.



```C++
OpenSkyhawk::Switch2Pos ejSafe(A_4E_C_SEAT_EJECT_SAFE, PA1);
```
 


    
## Public Functions Documentation




### function Switch2Pos 

_Construct a_ [_**Switch2Pos**_](classOpenSkyhawk_1_1Switch2Pos.md) _input object._
```C++
OpenSkyhawk::Switch2Pos::Switch2Pos (
    uint16_t addr,
    uint8_t pin
) 
```





**Parameters:**


* `addr` controlId to send in the CAN event (typically the DCS-BIOS address). 
* `pin` GPIO pin to read (configured INPUT\_PULLUP by the constructor). 




        

<hr>



### function poll 

_Read pin, debounce, and call_ [_**PanelGroup::sendEvent()**_](namespacePanelGroup.md#function-sendevent) _on state change._
```C++
virtual void OpenSkyhawk::Switch2Pos::poll () override
```



Debounce window is 20 ms. Sends value=1 when pin is LOW (active), value=0 when HIGH. 


        
Implements [*OpenSkyhawk::InputBase::poll*](classOpenSkyhawk_1_1InputBase.md#function-poll)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PanelGroup.h`

