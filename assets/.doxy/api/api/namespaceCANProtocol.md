

# Namespace CANProtocol



[**Namespace List**](namespaces.md) **>** [**CANProtocol**](namespaceCANProtocol.md)




























## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint32\_t | [**DUP\_NODE\_TIMEOUT\_MS**](#variable-dup_node_timeout_ms)   = `3000`<br>_Duplicate NODE\_ID clear time (#344): DUPLICATE\_NODE\_ID drops this long after the last foreign frame on our own_ `HB_NODE_ID` _— 6× the 500 ms HB cadence, matching_[_**PanelBridge**_](namespacePanelBridge.md) _'s own node timeout. A live duplicate re-raises within one HB period._ |
|  constexpr uint32\_t | [**MASTER\_TIMEOUT\_MS**](#variable-master_timeout_ms)   = `1500`<br>_Master-loss timeout: 3× the 500 ms_ `HB_0` _cadence, matching_[_**PanelBridge**_](namespacePanelBridge.md) _'s own node timeout ratio (3000 ms / 500 ms)._ |














## Public Functions

| Type | Name |
| ---: | :--- |
|  bool | [**busOff**](#function-busoff) () <br>_Return true if the CAN controller is in bus-off state._  |
|  void | [**drain**](#function-drain) () <br>_Process RX callbacks and batched-ControlPacket deadlines. Call once per loop()._  |
|  void | [**filterAcceptAll**](#function-filteracceptall) () <br>_Accept all incoming CAN frames. Use for_ [_**PanelBridge**_](namespacePanelBridge.md) _._ |
|  void | [**filterAcceptId**](#function-filteracceptid) (uint32\_t canId) <br>_Accept a specific CAN ID. Use for_ [_**PanelGroup**_](namespacePanelGroup.md) _nodes._ |
|  void | [**flushBatched**](#function-flushbatched) (uint32\_t canId) <br>_Force a half-full ControlPacketPair batch to send immediately._  |
|  const NodeHealthPayload & | [**lastNodeHealth**](#function-lastnodehealth) () <br>_The HEALTH\_n payload_ [_**serviceNodeStatus()**_](namespaceCANProtocol.md#function-servicenodestatus) _last built for this node (#344)._ |
|  HeartbeatPayload | [**makeHeartbeatPayload**](#function-makeheartbeatpayload) (uint8\_t nodeId, uint16\_t rxCount) <br>_Build the standard 8-byte heartbeat payload for the current node._  |
|  NodeHealthPayload | [**makeNodeHealthPayload**](#function-makenodehealthpayload) (uint8\_t nodeId, [**NodeKind**](NodeStatus_8h.md#enum-nodekind) kind, int8\_t dieTempC, [**NodeFaultCode**](NodeStatus_8h.md#enum-nodefaultcode) fault=NodeFaultCode::NONE) <br>_Build the 8-byte node-health payload for the current node._  |
|  void | [**onReceive**](#function-onreceive) ([**CanRxCallback**](CANProtocol_8h.md#typedef-canrxcallback) cb) <br>_Register a general-purpose RX frame handler._  |
|  void | [**onStatusChange**](#function-onstatuschange) ([**CanStatusCallback**](CANProtocol_8h.md#typedef-canstatuscallback) cb) <br>_Register a CAN bus status change callback._  |
|  void | [**onSyncReq**](#function-onsyncreq) ([**CanSyncReqCallback**](CANProtocol_8h.md#typedef-cansyncreqcallback) cb) <br>_Register a SYNC\_REQ handler._  |
|  uint8\_t | [**rec**](#function-rec) () <br>_Return the CAN Receive Error Counter (0-255) from ESR._  |
|  void | [**send**](#function-send) (uint32\_t canId, const uint8\_t \* data, uint8\_t len) <br>_Send a CAN frame._  |
|  void | [**sendBatched**](#function-sendbatched) (uint32\_t canId, const ControlPacket & pkt) <br>_Submit one ControlPacket to a CANProtocol-owned ControlPacketPair batch._  |
|  void | [**serviceNodeStatus**](#function-servicenodestatus) ([**NodeKind**](NodeStatus_8h.md#enum-nodekind) kind, uint16\_t rxCount=0) <br>_Run the node-generic status duties. Call once per loop(), after_ [_**drain()**_](namespaceCANProtocol.md#function-drain) _._ |
|  void | [**start**](#function-start) () <br>_Start the CAN peripheral and apply registered filters._  |
|  void | [**startLoopback**](#function-startloopback) () <br>_Start in silent loopback mode — for bench testing only._  |
|  uint8\_t | [**tec**](#function-tec) () <br>_Return the CAN Transmit Error Counter (0-255) from ESR._  |
|  uint32\_t | [**txDropCount**](#function-txdropcount) () <br>_Return the cumulative TX queue drop count since startup._  |




























## Public Static Attributes Documentation




### variable DUP\_NODE\_TIMEOUT\_MS 

_Duplicate NODE\_ID clear time (#344): DUPLICATE\_NODE\_ID drops this long after the last foreign frame on our own_ `HB_NODE_ID` _— 6× the 500 ms HB cadence, matching_[_**PanelBridge**_](namespacePanelBridge.md) _'s own node timeout. A live duplicate re-raises within one HB period._
```C++
constexpr uint32_t CANProtocol::DUP_NODE_TIMEOUT_MS;
```




<hr>



### variable MASTER\_TIMEOUT\_MS 

_Master-loss timeout: 3× the 500 ms_ `HB_0` _cadence, matching_[_**PanelBridge**_](namespacePanelBridge.md) _'s own node timeout ratio (3000 ms / 500 ms)._
```C++
constexpr uint32_t CANProtocol::MASTER_TIMEOUT_MS;
```




<hr>
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
* HB\_0 on a non-bridge node: stamped for [**serviceNodeStatus()**](namespaceCANProtocol.md#function-servicenodestatus)'s watchdog, then [**onReceive()**](namespaceCANProtocol.md#function-onreceive).
* The node's own `HB_NODE_ID`: stamped for [**serviceNodeStatus()**](namespaceCANProtocol.md#function-servicenodestatus)'s duplicate NODE\_ID check (#344), then [**onReceive()**](namespaceCANProtocol.md#function-onreceive).
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


* `canId` CAN\_ID\_CTRL\_BCAST, canIdEvt(NODE\_ID), canIdEvtRel(NODE\_ID), canIdEvtDir(NODE\_ID), or canIdEvtAction(NODE\_ID). 




        

<hr>



### function lastNodeHealth 

_The HEALTH\_n payload_ [_**serviceNodeStatus()**_](namespaceCANProtocol.md#function-servicenodestatus) _last built for this node (#344)._
```C++
const NodeHealthPayload & CANProtocol::lastNodeHealth () 
```



[**PanelBridge**](namespacePanelBridge.md) reports it as its own node-0 `_NODE_STATUS` row. Before the first HEALTH\_n (or with `-DNODE_HEALTH_TELEM=0`) it reads not-yet-seen: dieTempC DIE\_TEMP\_UNAVAILABLE, everything else 0 (nodeKind UNKNOWN) — the same rule the bridge applies to a node it has no HEALTH\_n from. 


        

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


* `nodeId` Node ID (1-63 [**PanelGroup**](namespacePanelGroup.md); GATEWAY\_NODE\_ID for [**PanelBridge**](namespacePanelBridge.md)). 
* `rxCount` Caller-owned receive counter to embed in the payload. 



**Returns:**

Fully populated HeartbeatPayload ready to send as HB\_n. 





        

<hr>



### function makeNodeHealthPayload 

_Build the 8-byte node-health payload for the current node._ 
```C++
NodeHealthPayload CANProtocol::makeNodeHealthPayload (
    uint8_t nodeId,
    NodeKind kind,
    int8_t dieTempC,
    NodeFaultCode fault=NodeFaultCode::NONE
) 
```



The CAN/DCS packing boundary: takes the internal die temperature (via [**STM32Board::readDieTempC()**](namespaceSTM32Board.md#function-readdietempc)) and the node's aggregated fault (via NodeStatus::aggregateFaults()) and packs them into a NodeHealthPayload. Sets the overheat flag (bit0) only when NODE\_OVERHEAT\_C is defined and dieTempC meets it; sets the DEGRADED flag (bit1) and faultId when `fault` is not NONE (the enum→byte cast happens here), and nodeKind from `kind`. faultMask + reserved bytes stay 0.




**Parameters:**


* `nodeId` Node ID (1-63 [**PanelGroup**](namespacePanelGroup.md); GATEWAY\_NODE\_ID [**PanelBridge**](namespacePanelBridge.md)). 
* `kind` The node's board kind (#344) — from its firmware role, never from nodeId. 
* `dieTempC` Internal die temp in whole °C (DIE\_TEMP\_UNAVAILABLE = unavailable). 
* `fault` Aggregated node fault (NodeFaultCode::NONE = healthy). Default NONE. 



**Returns:**

Fully populated NodeHealthPayload ready to send as HEALTH\_n. 





        

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



Valid only for CAN\_ID\_CTRL\_BCAST and the batched event frames — canIdEvt(n) (absolute), canIdEvtRel(n) (RotaryEncoder REL), canIdEvtDir(n) (RotaryEncoder DIR), and canIdEvtAction(n) (ActionButton) — each with its own pending slot. Pairs two consecutive packets into one 8-byte frame. If slot B does not arrive within two [**drain()**](namespaceCANProtocol.md#function-drain) calls, slot A is sent with slot B set to the null sentinel (controlId == 0x0000).


A frame ID with no batch slot is **silently discarded** — adding a batched frame means registering it in \_batches[] in begin(), not just defining its canId helper.




**Parameters:**


* `canId` CAN\_ID\_CTRL\_BCAST, canIdEvt(NODE\_ID), canIdEvtRel(NODE\_ID), canIdEvtDir(NODE\_ID), or canIdEvtAction(NODE\_ID). 
* `pkt` ControlPacket to batch. 




        

<hr>



### function serviceNodeStatus 

_Run the node-generic status duties. Call once per loop(), after_ [_**drain()**_](namespaceCANProtocol.md#function-drain) _._
```C++
void CANProtocol::serviceNodeStatus (
    NodeKind kind,
    uint16_t rxCount=0
) 
```



Every node on the bus owes the same status traffic, whatever else it does (#340):
* `HB_NODE_ID` every 500 ms (makeHeartbeatPayload), carrying `rxCount`.
* `HEALTH_NODE_ID` every 1000 ms (makeNodeHealthPayload) with `kind`, the internal die temp and the node fault, plus the DiagSerial fault edge log (tag `BRIDGE` on NODE\_ID 0, `NODE` otherwise). Compiled out with `-DNODE_HEALTH_TELEM=0`.
* Duplicate NODE\_ID check (#344), on every node: a node never receives its own frames, so a frame on its own `HB_NODE_ID` ([**start()**](namespaceCANProtocol.md#function-start) adds it to the filter) means another board has the same NODE\_ID. The node fault is then `NodeFaultCode::DUPLICATE_NODE_ID`, ahead of every FaultSource (the NODE\_ID is wrong, so nothing else this ID reports can be trusted); it clears DUP\_NODE\_TIMEOUT\_MS after the last foreign frame. aggregateFaults() supplies the fault otherwise.
* Master-loss watchdog, on every node except [**PanelBridge**](namespacePanelBridge.md) (NODE\_ID 0): once a master heartbeat (`HB_0`) has been seen, master loss is set after MASTER\_TIMEOUT\_MS of silence and cleared by the next `HB_0`.
* WARNING on a non-bridge node = master loss OR overheat (overheat only when a build defines `NODE_OVERHEAT_C`). The service tracks the two causes separately and writes the shared latch only when their OR changes, so a returning master never clears a live overheat, and overheat clears once the die cools. On [**PanelBridge**](namespacePanelBridge.md) an overheat is raise-only, because its latch also carries [**PanelBridge**](namespacePanelBridge.md)'s dead-node WARNING.




The first call arms the HB/HEALTH timers — the first loop() after setup(), i.e. boot complete — so the first HB goes out ~500 ms and the first HEALTH ~1000 ms after it.


`HB_0` arrival is recorded by [**drain()**](namespaceCANProtocol.md#function-drain), which still passes the frame to [**onReceive()**](namespaceCANProtocol.md#function-onreceive). [**start()**](namespaceCANProtocol.md#function-start) adds `HB_0` to the hardware filter on non-bridge nodes, so callers never register it by hand.




**Parameters:**


* `kind` This node's board kind (#344), sent in every HEALTH\_n. Each node library passes its own ([**PanelGroup**](namespacePanelGroup.md) → PANEL\_GROUP, [**PduNode**](namespacePduNode.md) → PDU, [**PanelBridge**](namespacePanelBridge.md) → BRIDGE), so a sketch never sets it. Required: a role can't forget it. 
* `rxCount` Caller-owned accepted RX count for the heartbeat ([**PanelGroup**](namespacePanelGroup.md): accepted CTRL\_BCAST frames). Pass 0 when the node keeps no count.



**Note:**

Samples millis() itself, after [**drain()**](namespaceCANProtocol.md#function-drain) has stamped any `HB_0`, and compares elapsed time signed — so a millisecond ticking during [**drain()**](namespaceCANProtocol.md#function-drain) cannot read as a timeout (#225). 





        

<hr>



### function start 

_Start the CAN peripheral and apply registered filters._ 
```C++
void CANProtocol::start () 
```



Must be called after [**STM32Board::begin()**](namespaceSTM32Board.md#function-begin) and after all filter and callback registrations. Always adds CTRL\_BCAST, TEST\_SEQ, and SYNC\_REQ to the active filter — mandatory for all nodes, cannot be excluded — plus HB\_0 on every node except [**PanelBridge**](namespacePanelBridge.md) (NODE\_ID 0), for [**serviceNodeStatus()**](namespaceCANProtocol.md#function-servicenodestatus)'s master-loss watchdog, and the node's own `HB_NODE_ID`, for its duplicate NODE\_ID check (#344).


Resets the [**serviceNodeStatus()**](namespaceCANProtocol.md#function-servicenodestatus) timers, which arm on its first call. Fires the onStatusChange callback with NORMAL on success. 


        

<hr>



### function startLoopback 

_Start in silent loopback mode — for bench testing only._ 
```C++
void CANProtocol::startLoopback () 
```



Identical to [**start()**](namespaceCANProtocol.md#function-start) but uses CAN\_MODE\_SILENT\_LOOPBACK: frames transmitted by this node are received back internally without going on the physical bus. The duplicate NODE\_ID check discounts the node's own looped-back heartbeats, so only an extra `HB_NODE_ID` a test sends itself reads as a duplicate.




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

