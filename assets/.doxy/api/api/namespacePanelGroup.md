

# Namespace PanelGroup



[**Namespace List**](namespaces.md) **>** [**PanelGroup**](namespacePanelGroup.md)



_Static singleton for CAN sub-node (_ [_**PanelGroup**_](namespacePanelGroup.md) _) firmware._[More...](#detailed-description)






































## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**loop**](#function-loop) () <br>_Service all_ [_**PanelGroup**_](namespacePanelGroup.md) _activity. Call every Arduino_[_**loop()**_](namespacePanelGroup.md#function-loop) _iteration._ |
|  uint8\_t | [**nodeId**](#function-nodeid) () <br>_Return the node ID read from the PA0 strap pin at boot._  |
|  bool | [**sendEvent**](#function-sendevent) (uint16\_t controlId, uint16\_t value) <br>_Send a hardware input event over CAN to the master node._  |
|  void | [**setup**](#function-setup) () <br>_Initialise hardware and start the CAN bus._  |




























## Detailed Description


Reads node\_id from the PA0 strap pin at boot, configures the CAN receive filter for CTRL\_BCAST and TEST\_SEQ frames, dispatches incoming frames to registered [**OpenSkyhawk**](namespaceOpenSkyhawk.md) output objects, polls registered input objects, and sends heartbeat frames every 500 ms.


Node ID strapping: tie PA0 to 3.3 V for node\_id=1; leave floating (internal pull-down) for node\_id=2. 


    
## Public Functions Documentation




### function loop 

_Service all_ [_**PanelGroup**_](namespacePanelGroup.md) _activity. Call every Arduino_[_**loop()**_](namespacePanelGroup.md#function-loop) _iteration._
```C++
void PanelGroup::loop () 
```



Calls STM32Board::update(), drains the CAN RX FIFO and dispatches ControlPacket frames to registered OutputBase objects, polls all InputBase objects, and sends a heartbeat CAN frame every 500 ms. 


        

<hr>



### function nodeId 

_Return the node ID read from the PA0 strap pin at boot._ 
```C++
uint8_t PanelGroup::nodeId () 
```





**Returns:**

1 if PA0 was HIGH, 2 if PA0 was LOW. 





        

<hr>



### function sendEvent 

_Send a hardware input event over CAN to the master node._ 
```C++
bool PanelGroup::sendEvent (
    uint16_t controlId,
    uint16_t value
) 
```



Packs controlId and value into an 8-byte CAN frame on CAN\_ID\_EVT\_n (where n = [**nodeId()**](namespacePanelGroup.md#function-nodeid)). Intended to be called by InputBase subclasses.




**Parameters:**


* `controlId` Identifies the physical control (HID range 0x0010–0x00FF, or DCS-BIOS address 0x8000+). 
* `value` Current control value (0/1 for switches, 0–65535 for axes). 



**Returns:**

true on successful TX, false if the CAN mailbox was full. 





        

<hr>



### function setup 

_Initialise hardware and start the CAN bus._ 
```C++
void PanelGroup::setup () 
```



Calls [**STM32Board::begin()**](namespaceSTM32Board.md#function-begin), reads the PA0 strap pin to set node\_id, configures the CAN filter (CTRL\_BCAST + TEST\_SEQ), then calls canStart(). Call STM32Board::setDebug(true) before this to enable diagnostic output. 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PanelGroup.cpp`

