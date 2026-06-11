

# Class OpenSkyhawk::HIDHatSwitch



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**HIDHatSwitch**](classOpenSkyhawk_1_1HIDHatSwitch.md)



_HID hat switch handler. Declared at sketch scope for each hat switch._ [More...](#detailed-description)

* `#include <SimGateway.h>`





































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**HIDHatSwitch**](#function-hidhatswitch) (uint16\_t controlId, uint8\_t hatIndex) <br>_Register a HID hat switch handler._  |
|  uint16\_t | [**controlId**](#function-controlid) () const<br> |
|  void | [**dispatch**](#function-dispatch) (uint16\_t value) <br>_Dispatch a direction to the hat switch._  |
|  [**HIDHatSwitch**](classOpenSkyhawk_1_1HIDHatSwitch.md) \* | [**next**](#function-next) () const<br> |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  [**HIDHatSwitch**](classOpenSkyhawk_1_1HIDHatSwitch.md) \* | [**head**](#function-head) () <br> |


























## Detailed Description


Self-registers into a static linked list at construction. Dispatches to OsJoystick.setHat() with direction clamped to 0–8. 


    
## Public Functions Documentation




### function HIDHatSwitch 

_Register a HID hat switch handler._ 
```C++
OpenSkyhawk::HIDHatSwitch::HIDHatSwitch (
    uint16_t controlId,
    uint8_t hatIndex
) 
```





**Parameters:**


* `controlId` CTRL\_\* constant from [**HIDControls.h**](HIDControls_8h.md) (0x0020–0x002F range). 
* `hatIndex` OpenSkyhawkJoystick hat index (0–3). 




        

<hr>



### function controlId 

```C++
uint16_t OpenSkyhawk::HIDHatSwitch::controlId () const
```




<hr>



### function dispatch 

_Dispatch a direction to the hat switch._ 
```C++
void OpenSkyhawk::HIDHatSwitch::dispatch (
    uint16_t value
) 
```





**Parameters:**


* `value` 0 = centered, 1 = N, 2 = NE, 3 = E, 4 = SE, 5 = S, 6 = SW, 7 = W, 8 = NW. Values &gt; 8 → centered. 




        

<hr>



### function next 

```C++
HIDHatSwitch * OpenSkyhawk::HIDHatSwitch::next () const
```




<hr>
## Public Static Functions Documentation




### function head 

```C++
static HIDHatSwitch * OpenSkyhawk::HIDHatSwitch::head () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/SimGateway.h`

