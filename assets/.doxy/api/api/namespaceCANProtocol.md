

# Namespace CANProtocol



[**Namespace List**](namespaces.md) **>** [**CANProtocol**](namespaceCANProtocol.md)










































## Public Functions

| Type | Name |
| ---: | :--- |
|  bool | [**busOff**](#function-busoff) () <br>_Return true if the CAN controller is in bus-off state._  |
|  void | [**drain**](#function-drain) () <br>_Process RX callbacks and batched-ControlPacket deadlines. Call once per loop()._  |
|  void | [**filterAcceptAll**](#function-filteracceptall) () <br>_Accept all incoming CAN frames. Use for_ [_**PanelBridge**_](namespacePanelBridge.md) _._ |
|  void | [**filterAcceptId**](#function-filteracceptid) (uint32\_t canId) <br>_Accept a specific CAN ID. Use for_ [_**PanelGroup**_](namespacePanelGroup.md) _nodes._ |
|  void | [**flushBatched**](#function-flushbatched) (uint32\_t canId) <br>_Force a half-full ControlPacketPair batch to send immediately._  |
|  HeartbeatPayload | [**makeHeartbeatPayload**](#function-makeheartbeatpayload) (uint8\_t nodeId, uint16\_t rxCount) <br>_Build the standard 8-byte heartbeat payload for the current node._  |
|  void | [**onReceive**](#function-onreceive) ([**CanRxCallback**](CANProtocol_8h.md#typedef-canrxcallback) cb) <br>_Register a general-purpose RX frame handler._  |
|  void | [**onStatusChange**](#function-onstatuschange) ([**CanStatusCallback**](CANProtocol_8h.md#typedef-canstatuscallback) cb) <br>_Register a CAN bus status change callback._  |
|  void | [**onSyncReq**](#function-onsyncreq) ([**CanSyncReqCallback**](CANProtocol_8h.md#typedef-cansyncreqcallback) cb) <br>_Register a SYNC\_REQ handler._  |
|  uint8\_t | [**rec**](#function-rec) () <br>_Return the CAN Receive Error Counter (0-255) from ESR._  |
|  void | [**send**](#function-send) (uint32\_t canId, const uint8\_t \* data, uint8\_t len) <br>_Send a CAN frame._  |
|  void | [**sendBatched**](#function-sendbatched) (uint32\_t canId, const ControlPacket & pkt) <br>_Submit one ControlPacket to a CANProtocol-owned ControlPacketPair batch._  |
|  void | [**start**](#function-start) () <br>_Start the CAN peripheral and apply registered filters._  |
|  void | [**startLoopback**](#function-startloopback) () <br>_Start in silent loopback mode — for bench testing only._  |
|  uint8\_t | [**tec**](#function-tec) () <br>_Return the CAN Transmit Error Counter (0-255) from ESR._  |
|  uint32\_t | [**txDropCount**](#function-txdropcount) () <br>_Return the cumulative TX queue drop count since startup._  |




























## Public Functions Documentation




### function busOff 

_Return true if the CAN controller is in bus-off state._ 
```C++
bool CANProtocol::busOff () 
```




<hr>



### function drain 

_Process RX callbacks and batched-ControlPacket deadlines. Call once per loop()._ 
```C++
void CANProtocol::drain () 
```



Drains all frames received since the last call:
* SYNC\_REQ: fires [**onSyncReq()**](namespaceCANProtocol.md#function-onsyncreq), not forwarded to [**onReceive()**](namespaceCANProtocol.md#function-onreceive).
* TEST\_SEQ: auto-replies with ECHO carrying the same payload, not forwarded.
* All other frames: fires [**onReceive()**](namespaceCANProtocol.md#function-onreceive).




Also services [**sendBatched()**](namespaceCANProtocol.md#function-sendbatched) deadlines: a half-full ControlPacketPair that has waited two [**drain()**](namespaceCANProtocol.md#function-drain) calls is flushed with slot B set to the null sentinel. 


        

<hr>



### function filterAcceptAll 

_Accept all incoming CAN frames. Use for_ [_**PanelBridge**_](namespacePanelBridge.md) _._
```C++
void CANProtocol::filterAcceptAll () 
```



Configures a pass-all hardware mask filter. Mandatory IDs (CTRL\_BCAST, TEST\_SEQ, SYNC\_REQ) are included but have no effect with pass-all active. 


        

<hr>



### function filterAcceptId 

_Accept a specific CAN ID. Use for_ [_**PanelGroup**_](namespacePanelGroup.md) _nodes._
```C++
void CANProtocol::filterAcceptId (
    uint32_t canId
) 
```



Adds one ID to the hardware filter list. Call multiple times for multiple IDs. CTRL\_BCAST (0x010), TEST\_SEQ (0x011), and SYNC\_REQ (0x012) are always included automatically by [**start()**](namespaceCANProtocol.md#function-start) — do not add them manually.




**Parameters:**


* `canId` 11-bit standard CAN ID to accept. 




        

<hr>



### function flushBatched 

_Force a half-full ControlPacketPair batch to send immediately._ 
```C++
void CANProtocol::flushBatched (
    uint32_t canId
) 
```



If the named CAN ID has a pending slot A, sends it with slot B as the null sentinel. No-op if no packet is pending.




**Parameters:**


* `canId` CAN\_ID\_CTRL\_BCAST or canIdEvt(NODE\_ID). 




        

<hr>



### function makeHeartbeatPayload 

_Build the standard 8-byte heartbeat payload for the current node._ 
```C++
HeartbeatPayload CANProtocol::makeHeartbeatPayload (
    uint8_t nodeId,
    uint16_t rxCount
) 
```



Fills uptime, CAN health flags, and ESR-derived TEC/REC from [**CANProtocol**](namespaceCANProtocol.md) state.




**Parameters:**


* `nodeId` Node ID (1-63 [**PanelGroup**](namespacePanelGroup.md); 0 reserved for [**PanelBridge**](namespacePanelBridge.md)). 
* `rxCount` Caller-owned receive counter to embed in the payload. 



**Returns:**

Fully populated HeartbeatPayload ready to send as HB\_n. 





        

<hr>



### function onReceive 

_Register a general-purpose RX frame handler._ 
```C++
void CANProtocol::onReceive (
    CanRxCallback cb
) 
```



Fired by [**drain()**](namespaceCANProtocol.md#function-drain) for all frames except SYNC\_REQ and TEST\_SEQ. Only one handler per node — [**PanelGroup**](namespacePanelGroup.md) and [**PanelBridge**](namespacePanelBridge.md) each register their own.




**Parameters:**


* `cb` Callback invoked with canId, data pointer, and payload length. 




        

<hr>



### function onStatusChange 

_Register a CAN bus status change callback._ 
```C++
void CANProtocol::onStatusChange (
    CanStatusCallback cb
) 
```



Fires immediately with the current status upon registration (STARTING before [**start()**](namespaceCANProtocol.md#function-start) is called), then on each subsequent status transition.




**Parameters:**


* `cb` Callback to invoke with the new CanStatus. 




        

<hr>



### function onSyncReq 

_Register a SYNC\_REQ handler._ 
```C++
void CANProtocol::onSyncReq (
    CanSyncReqCallback cb
) 
```



Fired by [**drain()**](namespaceCANProtocol.md#function-drain) when SYNC\_REQ (0x012) is received. [**PanelGroup**](namespacePanelGroup.md) registers this to trigger a re-poll of all registered input objects.




**Parameters:**


* `cb` Callback to invoke on SYNC\_REQ receipt. 




        

<hr>



### function rec 

_Return the CAN Receive Error Counter (0-255) from ESR._ 
```C++
uint8_t CANProtocol::rec () 
```




<hr>



### function send 

_Send a CAN frame._ 
```C++
void CANProtocol::send (
    uint32_t canId,
    const uint8_t * data,
    uint8_t len
) 
```



Sends immediately if a TX mailbox is free; otherwise enqueues in the TX ring buffer (~16 entries), drained automatically via TX-complete interrupt. CTRL\_BCAST coalesces stale queued state (newest wins). EVT/control frames retry up to 3 attempts then drop. All drops increment [**txDropCount()**](namespaceCANProtocol.md#function-txdropcount).




**Parameters:**


* `canId` 11-bit standard CAN ID. 
* `data` Pointer to payload bytes. 
* `len` Payload length in bytes (0-8). 




        

<hr>



### function sendBatched 

_Submit one ControlPacket to a CANProtocol-owned ControlPacketPair batch._ 
```C++
void CANProtocol::sendBatched (
    uint32_t canId,
    const ControlPacket & pkt
) 
```



Valid only for CAN\_ID\_CTRL\_BCAST and canIdEvt(n). Pairs two consecutive packets into one 8-byte frame. If slot B does not arrive within two [**drain()**](namespaceCANProtocol.md#function-drain) calls, slot A is sent with slot B set to the null sentinel (controlId == 0x0000).




**Parameters:**


* `canId` CAN\_ID\_CTRL\_BCAST or canIdEvt(NODE\_ID). 
* `pkt` ControlPacket to batch. 




        

<hr>



### function start 

_Start the CAN peripheral and apply registered filters._ 
```C++
void CANProtocol::start () 
```



Must be called after [**STM32Board::begin()**](namespaceSTM32Board.md#function-begin) and after all filter and callback registrations. Always adds CTRL\_BCAST, TEST\_SEQ, and SYNC\_REQ to the active filter — mandatory for all nodes, cannot be excluded.


Fires the onStatusChange callback with NORMAL on success. 


        

<hr>



### function startLoopback 

_Start in silent loopback mode — for bench testing only._ 
```C++
void CANProtocol::startLoopback () 
```



Identical to [**start()**](namespaceCANProtocol.md#function-start) but uses CAN\_MODE\_SILENT\_LOOPBACK: frames transmitted by this node are received back internally without going on the physical bus.




**Note:**

Never call this in production firmware. 





        

<hr>



### function tec 

_Return the CAN Transmit Error Counter (0-255) from ESR._ 
```C++
uint8_t CANProtocol::tec () 
```




<hr>



### function txDropCount 

_Return the cumulative TX queue drop count since startup._ 
```C++
uint32_t CANProtocol::txDropCount () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/CANProtocol/CANProtocol.cpp`

