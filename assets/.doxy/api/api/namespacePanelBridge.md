

# Namespace PanelBridge



[**Namespace List**](namespaces.md) **>** [**PanelBridge**](namespacePanelBridge.md)






















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\*)(uint8\_t nodeId) | [**NodeCallback**](#typedef-nodecallback)  <br>_Callback type for node liveness events. Parameter is_ [_**PanelGroup**_](namespacePanelGroup.md) _node ID (1–63)._ |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**loop**](#function-loop) () <br>_Run_ [_**PanelBridge**_](namespacePanelBridge.md) _work not owned by DcsBios::loop()._ |
|  void | [**onNodeAlive**](#function-onnodealive) ([**NodeCallback**](namespacePanelBridge.md#typedef-nodecallback) cb) <br>_Register a callback fired when a node transitions from dead/unseen to alive._  |
|  void | [**onNodeDead**](#function-onnodedead) ([**NodeCallback**](namespacePanelBridge.md#typedef-nodecallback) cb) <br>_Register a callback fired when a live node misses the heartbeat timeout (3 s)._  |
|  void | [**setup**](#function-setup) () <br>_Initialise STM32 board services, UART2,_ [_**CANProtocol**_](namespaceCANProtocol.md) _, and_[_**PanelBridge**_](namespacePanelBridge.md) _internals._ |




























## Public Types Documentation




### typedef NodeCallback 

_Callback type for node liveness events. Parameter is_ [_**PanelGroup**_](namespacePanelGroup.md) _node ID (1–63)._
```C++
using PanelBridge::NodeCallback = typedef void(*)(uint8_t nodeId);
```




<hr>
## Public Functions Documentation




### function loop 

_Run_ [_**PanelBridge**_](namespacePanelBridge.md) _work not owned by DcsBios::loop()._
```C++
void PanelBridge::loop () 
```



Drains [**CANProtocol**](namespaceCANProtocol.md) RX, dispatches EVT packets, services [**CANProtocol**](namespaceCANProtocol.md) batching deadlines, checks node heartbeat timeouts (3 s), and handles DiagSerial 'T' bytes for TEST\_SEQ.




**Note:**

Call after DcsBios::loop() each iteration. 





        

<hr>



### function onNodeAlive 

_Register a callback fired when a node transitions from dead/unseen to alive._ 
```C++
void PanelBridge::onNodeAlive (
    NodeCallback cb
) 
```



Passing nullptr clears the callback. Set before calling [**setup()**](namespacePanelBridge.md#function-setup).




**Parameters:**


* `cb` Function called with the [**PanelGroup**](namespacePanelGroup.md) node ID (1–63). 




        

<hr>



### function onNodeDead 

_Register a callback fired when a live node misses the heartbeat timeout (3 s)._ 
```C++
void PanelBridge::onNodeDead (
    NodeCallback cb
) 
```



Passing nullptr clears the callback. Set before calling [**setup()**](namespacePanelBridge.md#function-setup).




**Parameters:**


* `cb` Function called with the [**PanelGroup**](namespacePanelGroup.md) node ID (1–63). 




        

<hr>



### function setup 

_Initialise STM32 board services, UART2,_ [_**CANProtocol**_](namespaceCANProtocol.md) _, and_[_**PanelBridge**_](namespacePanelBridge.md) _internals._
```C++
void PanelBridge::setup () 
```



Performs in order: [**STM32Board::begin()**](namespaceSTM32Board.md#function-begin), Serial.begin(250000), DCS-BIOS export listener registration, [**CANProtocol::filterAcceptAll()**](namespaceCANProtocol.md#function-filteracceptall), [**CANProtocol::onReceive()**](namespaceCANProtocol.md#function-onreceive) registration, [**CANProtocol::start()**](namespaceCANProtocol.md#function-start), and cold-boot SYNC\_REQ broadcast.




**Note:**

Sketch must call DcsBios::setup() immediately after this. 





        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelBridge/PanelBridge.cpp`

