

# File NodeStatus.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**NodeStatus**](dir_9111f7d39b5fc785aa55dffe02a55e74.md) **>** [**NodeStatus.h**](NodeStatus_8h.md)

[Go to the source code of this file](NodeStatus_8h_source.md)

_Neutral node-status contract for every_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _node (_[_**PanelGroup**_](namespacePanelGroup.md) _,_[_**PanelBridge**_](namespacePanelBridge.md) _, PDU)._[More...](#detailed-description)

* `#include <stdint.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md) <br>_A source of node faults — implemented by any object that can fault (#163)._  |


## Public Types

| Type | Name |
| ---: | :--- |
| enum uint8\_t | [**NodeFaultCode**](#enum-nodefaultcode)  <br>_HEALTH\_n_ `faultId` _dictionary (#163) — cross-node fault codes._ |
| enum uint8\_t | [**NodeHealthFlag**](#enum-nodehealthflag)  <br>_HEALTH\_n_ `flags` _bits — node-level health conditions (maps to NodeHealthPayload.flags)._ |
| enum uint8\_t | [**NodeKind**](#enum-nodekind)  <br>_HEALTH\_n_ `nodeKind` _(byte 5) — what kind of board this node is (#344)._ |
| enum uint8\_t | [**PduRailFlag**](#enum-pdurailflag)  <br>_PDU\_RAIL\_\* page byte 1 — per-rail condition bits (all zero = healthy)._  |
| enum uint8\_t | [**PduStatusFlag**](#enum-pdustatusflag)  <br>_PDU\_STATUS page byte 1 — board condition bits (all zero = healthy, calibrated)._  |
| enum uint8\_t | [**TelemPage**](#enum-telempage)  <br>_TELEM\_n page ids (#216) — byte 0 of TelemPayload, naming the layout of bytes 1-7._  |















































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**NODE\_STATUS\_END\_MSG\_NAME**](NodeStatus_8h.md#define-node_status_end_msg_name)  `"\_NODE\_STATUS\_END"`<br> |
| define  | [**NODE\_STATUS\_MSG\_NAME**](NodeStatus_8h.md#define-node_status_msg_name)  `"\_NODE\_STATUS"`<br> |
| define  | [**NODE\_STATUS\_PROTO\_VERSION**](NodeStatus_8h.md#define-node_status_proto_version)  `3`<br> |
| define  | [**NODE\_STATUS\_REQ\_ADDR**](NodeStatus_8h.md#define-node_status_req_addr)  `0x86FE`<br> |
| define  | [**NODE\_TELEM\_MSG\_NAME**](NodeStatus_8h.md#define-node_telem_msg_name)  `"\_NODE\_TELEM"`<br> |
| define  | [**NODE\_TELEM\_PROTO\_VERSION**](NodeStatus_8h.md#define-node_telem_proto_version)  `1`<br> |

## Detailed Description


Owns the cross-node **status API** — not just health:
* the `_NODE_STATUS` DCS-BIOS host-reporting contract (proto version, request address, message names) — **the canonical source the client's `sync-a4ec.ts` parses**;
* `NodeHealthFlag` (HEALTH\_n flag bits) and `NodeFaultCode` (the compact wire faultId dictionary the client maps to labels);
* `NodeKind` (HEALTH\_n board-kind byte — what board a NODE\_ID is, announced by its firmware);
* the `_NODE_TELEM` host contract and the TELEM\_n page vocabulary (`TelemPage`, `PduRailFlag`, `PduStatusFlag`) — #216;
* `FaultSource` — the interface a fault-producing object implements so a node-level aggregator can roll it up. A `FaultSource` is _one piece_ of node status, not the whole story.




This is the neutral home for node-status vocabulary so it isn't shaped around any one node flavour (PanelGroup/OutputBase) or the HID namespace. `NodeHealthPayload` (the CAN HEALTH\_n frame struct) stays in `CANProtocol.h`; the fault _detail_ strings stay local (DiagSerial), never on the wire. Header-only contract + a tiny `FaultSource` registry (`NodeStatus.cpp`).




**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    
## Public Types Documentation




### enum NodeFaultCode 

_HEALTH\_n_ `faultId` _dictionary (#163) — cross-node fault codes._
```C++
enum NodeFaultCode {
    NONE = 0x00,
    I2C_PERIPHERAL = 0x01,
    OVER_VOLTAGE = 0x02,
    UNDER_VOLTAGE = 0x03,
    SHORT_CIRCUIT = 0x04,
    HOST_LINK_LOST = 0x05,
    FUSE_OPEN = 0x06,
    OVER_TEMPERATURE = 0x07,
    SENSOR_FAULT = 0x08,
    DUPLICATE_NODE_ID = 0x09
};
```



Coarse, one active at a time on the wire; the exact device is logged locally on DiagSerial, not the frame. The client maps id → human label (SkyHawkClient#40). Each node type contributes the codes relevant to its fault sources; append as new sources appear. 


        

<hr>



### enum NodeHealthFlag 

_HEALTH\_n_ `flags` _bits — node-level health conditions (maps to NodeHealthPayload.flags)._
```C++
enum NodeHealthFlag {
    OVERHEAT = 0x01,
    DEGRADED = 0x02
};
```




<hr>



### enum NodeKind 

_HEALTH\_n_ `nodeKind` _(byte 5) — what kind of board this node is (#344)._
```C++
enum NodeKind {
    UNKNOWN = 0x00,
    PANEL_GROUP = 0x01,
    PDU = 0x02,
    BRIDGE = 0x03
};
```



Announced by the board's own firmware role (each node library passes its kind to [**CANProtocol::serviceNodeStatus()**](namespaceCANProtocol.md#function-servicenodestatus)), never derived from the NODE\_ID — a PDU can take any ID. Append-only. 0x00 is what firmware older than #344 sends (the byte was reserved), so the client reads it as "unknown" and never adopts on it. 


        

<hr>



### enum PduRailFlag 

_PDU\_RAIL\_\* page byte 1 — per-rail condition bits (all zero = healthy)._ 
```C++
enum PduRailFlag {
    UNDER_V = 0x01,
    OVER_V = 0x02,
    OVER_CURRENT = 0x04,
    FUSE_OPEN = 0x08,
    SENSOR_FAULT = 0x10
};
```




<hr>



### enum PduStatusFlag 

_PDU\_STATUS page byte 1 — board condition bits (all zero = healthy, calibrated)._ 
```C++
enum PduStatusFlag {
    UNCAL_VDDA = 0x01,
    UNCAL_ZERO = 0x02,
    TEMP_WARN = 0x04,
    TEMP_CRIT = 0x08,
    NTC_FAULT = 0x10
};
```




<hr>



### enum TelemPage 

_TELEM\_n page ids (#216) — byte 0 of TelemPayload, naming the layout of bytes 1-7._ 
```C++
enum TelemPage {
    INVALID = 0x00,
    PDU_RAIL_12V = 0x01,
    PDU_RAIL_5V = 0x02,
    PDU_STATUS = 0x03
};
```



Global and append-only: a page id means the same layout whichever node sends it, so the client decodes by page, not by node type. 0x00 is invalid. Layouts (little-endian): PDU\_RAIL\_12V / PDU\_RAIL\_5V: [1] PduRailFlag bits · [2-3] vAvgMv u16 · [4-5] vMinMv u16 · [6-7] iAvgMa int16 PDU\_STATUS: [1] PduStatusFlag bits · [2-3] ntcTempDC int16 (0.1 °C, INT16\_MIN = unavailable) · [4-5] i12PeakMa u16 · [6-7] i5PeakMa u16 


        

<hr>
## Macro Definition Documentation





### define NODE\_STATUS\_END\_MSG\_NAME 

```C++
#define NODE_STATUS_END_MSG_NAME `"_NODE_STATUS_END"`
```




<hr>



### define NODE\_STATUS\_MSG\_NAME 

```C++
#define NODE_STATUS_MSG_NAME `"_NODE_STATUS"`
```




<hr>



### define NODE\_STATUS\_PROTO\_VERSION 

```C++
#define NODE_STATUS_PROTO_VERSION `3`
```




<hr>



### define NODE\_STATUS\_REQ\_ADDR 

```C++
#define NODE_STATUS_REQ_ADDR `0x86FE`
```




<hr>



### define NODE\_TELEM\_MSG\_NAME 

```C++
#define NODE_TELEM_MSG_NAME `"_NODE_TELEM"`
```




<hr>



### define NODE\_TELEM\_PROTO\_VERSION 

```C++
#define NODE_TELEM_PROTO_VERSION `1`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/NodeStatus/NodeStatus.h`

