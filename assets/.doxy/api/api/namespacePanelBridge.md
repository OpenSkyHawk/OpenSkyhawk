

# Namespace PanelBridge



[**Namespace List**](namespaces.md) **>** [**PanelBridge**](namespacePanelBridge.md)



_Static singleton for CAN master / UART bridge firmware._ [More...](#detailed-description)






































## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**loop**](#function-loop) () <br>_Service all_ [_**PanelBridge**_](namespacePanelBridge.md) _activity. Call every Arduino_[_**loop()**_](namespacePanelBridge.md#function-loop) _iteration._ |
|  void | [**onNodeAlive**](#function-onnodealive) (void(\*)(uint8\_t nodeId) cb) <br>_Register a callback invoked when a sub-node sends its first heartbeat after a dead or startup period._  |
|  void | [**onNodeDead**](#function-onnodedead) (void(\*)(uint8\_t nodeId) cb) <br>_Register a callback invoked when a sub-node has not sent a heartbeat for 3 seconds. Called once per timed-out node._  |
|  void | [**setup**](#function-setup) (HardwareSerial & uartPort) <br>_Initialise hardware and start the CAN bus._  |




























## Detailed Description


Bridges ControlPacket structs between the RP2040 [**SimGateway**](namespaceSimGateway.md) (UART) and the CAN bus. Accepts all CAN frames. Forwards heartbeats and RTT echo frames to the RP2040 as 8-byte DIAG frames (DIAG\_MAGIC framing defined in [**CANProtocol.h**](CANProtocol_8h.md)). Fires optional callbacks when a sub-node comes alive or goes silent. 


    
## Public Functions Documentation




### function loop 

_Service all_ [_**PanelBridge**_](namespacePanelBridge.md) _activity. Call every Arduino_[_**loop()**_](namespacePanelBridge.md#function-loop) _iteration._
```C++
void PanelBridge::loop () 
```



Calls STM32Board::update(), drains the UART RX buffer and processes incoming ControlPackets, drains the CAN RX FIFO and forwards frames to the RP2040, and checks the sub-node heartbeat watchdog. 


        

<hr>



### function onNodeAlive 

_Register a callback invoked when a sub-node sends its first heartbeat after a dead or startup period._ 
```C++
void PanelBridge::onNodeAlive (
    void(*)(uint8_t nodeId) cb
) 
```





**Note:**

Set before calling [**setup()**](namespacePanelBridge.md#function-setup). 




**Parameters:**


* `cb` Function called with the node\_id of the newly live node. 




        

<hr>



### function onNodeDead 

_Register a callback invoked when a sub-node has not sent a heartbeat for 3 seconds. Called once per timed-out node._ 
```C++
void PanelBridge::onNodeDead (
    void(*)(uint8_t nodeId) cb
) 
```





**Note:**

Set before calling [**setup()**](namespacePanelBridge.md#function-setup). 




**Parameters:**


* `cb` Function called with the node\_id (1 or 2) of the timed-out node. 




        

<hr>



### function setup 

_Initialise hardware and start the CAN bus._ 
```C++
void PanelBridge::setup (
    HardwareSerial & uartPort
) 
```



Calls [**STM32Board::begin()**](namespaceSTM32Board.md#function-begin), starts the UART at 250000 baud, configures a pass-all CAN filter, then calls canStart(). Call STM32Board::setDebug(true) before this to enable diagnostic output.




**Parameters:**


* `uartPort` Hardware serial port connected to the RP2040 [**SimGateway**](namespaceSimGateway.md). Use Serial2 (UART2 PA2/PA3) on the standard master board. 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelBridge/PanelBridge.cpp`

