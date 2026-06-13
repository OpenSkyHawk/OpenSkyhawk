

# Class OpenSkyhawk::HIDButton



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**HIDButton**](classOpenSkyhawk_1_1HIDButton.md)



_HID button handler. Declared at sketch scope for each button._ [More...](#detailed-description)

* `#include <SimGateway.h>`





































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**HIDButton**](#function-hidbutton) (uint16\_t controlId, uint8\_t buttonIndex) <br>_Register a HID button handler._  |
|  uint16\_t | [**controlId**](#function-controlid) () const<br> |
|  void | [**dispatch**](#function-dispatch) (uint16\_t value) <br>_Dispatch a value to the joystick button._  |
|  [**HIDButton**](classOpenSkyhawk_1_1HIDButton.md) \* | [**next**](#function-next) () const<br> |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  [**HIDButton**](classOpenSkyhawk_1_1HIDButton.md) \* | [**head**](#function-head) () <br> |


























## Detailed Description


Self-registers into a static linked list at construction. value != 0 → button pressed; value == 0 → button released. 


    
## Public Functions Documentation




### function HIDButton 

_Register a HID button handler._ 
```C++
OpenSkyhawk::HIDButton::HIDButton (
    uint16_t controlId,
    uint8_t buttonIndex
) 
```





**Parameters:**


* `controlId` CTRL\_\* button constant from [**HIDControls.h**](HIDControls_8h.md) (0x0030–0x00AF range). 
* `buttonIndex` OpenSkyhawkJoystick button index (0–127). 




        

<hr>



### function controlId 

```C++
uint16_t OpenSkyhawk::HIDButton::controlId () const
```




<hr>



### function dispatch 

_Dispatch a value to the joystick button._ 
```C++
void OpenSkyhawk::HIDButton::dispatch (
    uint16_t value
) 
```





**Parameters:**


* `value` 0 → released; non-zero → pressed. 




        

<hr>



### function next 

```C++
HIDButton * OpenSkyhawk::HIDButton::next () const
```




<hr>
## Public Static Functions Documentation




### function head 

```C++
static HIDButton * OpenSkyhawk::HIDButton::head () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/SimGateway.h`

