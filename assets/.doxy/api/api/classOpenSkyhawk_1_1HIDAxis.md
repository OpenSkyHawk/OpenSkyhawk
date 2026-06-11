

# Class OpenSkyhawk::HIDAxis



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**HIDAxis**](classOpenSkyhawk_1_1HIDAxis.md)



_HID axis handler. Declared at sketch scope for each joystick axis._ [More...](#detailed-description)

* `#include <SimGateway.h>`





































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**HIDAxis**](#function-hidaxis) (uint16\_t controlId, uint8\_t axisIndex) <br>_Register a HID axis handler._  |
|  uint16\_t | [**controlId**](#function-controlid) () const<br>_controlId this handler is registered for._  |
|  void | [**dispatch**](#function-dispatch) (uint16\_t value) <br>_Dispatch a value to the joystick axis._  |
|  [**HIDAxis**](classOpenSkyhawk_1_1HIDAxis.md) \* | [**next**](#function-next) () const<br>_Next axis in list; nullptr at end._  |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  [**HIDAxis**](classOpenSkyhawk_1_1HIDAxis.md) \* | [**head**](#function-head) () <br>_First registered axis; nullptr if none._  |


























## Detailed Description


Self-registers into a static linked list at construction. [**SimGateway::loop()**](namespaceSimGateway.md#function-loop) walks the list and calls OsJoystick.setAxis() when a HID frame with a matching controlId arrives. The 0–65535 unsigned value from [**PanelGroup**](namespacePanelGroup.md) is mapped to signed ±32767 internally (value − 32768).


Declare at file scope, not inside functions — C++ constructs file-scope objects before setup() runs, which is required for the linked list to be populated. 


    
## Public Functions Documentation




### function HIDAxis 

_Register a HID axis handler._ 
```C++
OpenSkyhawk::HIDAxis::HIDAxis (
    uint16_t controlId,
    uint8_t axisIndex
) 
```





**Parameters:**


* `controlId` CTRL\_\* constant from [**HIDControls.h**](HIDControls_8h.md) (0x0010–0x001F range). 
* `axisIndex` OpenSkyhawkJoystick axis index (0–7). 




        

<hr>



### function controlId 

_controlId this handler is registered for._ 
```C++
uint16_t OpenSkyhawk::HIDAxis::controlId () const
```




<hr>



### function dispatch 

_Dispatch a value to the joystick axis._ 
```C++
void OpenSkyhawk::HIDAxis::dispatch (
    uint16_t value
) 
```





**Parameters:**


* `value` Unsigned 0–65535; mapped to int16\_t (value − 32768) internally. 




        

<hr>



### function next 

_Next axis in list; nullptr at end._ 
```C++
HIDAxis * OpenSkyhawk::HIDAxis::next () const
```




<hr>
## Public Static Functions Documentation




### function head 

_First registered axis; nullptr if none._ 
```C++
static HIDAxis * OpenSkyhawk::HIDAxis::head () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/SimGateway.h`

