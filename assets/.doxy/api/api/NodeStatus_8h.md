

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















































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**NODE\_STATUS\_END\_MSG\_NAME**](NodeStatus_8h.md#define-node_status_end_msg_name)  `"\_NODE\_STATUS\_END"`<br> |
| define  | [**NODE\_STATUS\_MSG\_NAME**](NodeStatus_8h.md#define-node_status_msg_name)  `"\_NODE\_STATUS"`<br> |
| define  | [**NODE\_STATUS\_PROTO\_VERSION**](NodeStatus_8h.md#define-node_status_proto_version)  `2`<br> |
| define  | [**NODE\_STATUS\_REQ\_ADDR**](NodeStatus_8h.md#define-node_status_req_addr)  `0x86FE`<br> |

## Detailed Description


Owns the cross-node **status API** — not just health:
* the `_NODE_STATUS` DCS-BIOS host-reporting contract (proto version, request address, message names) — **the canonical source the client's `sync-a4ec.ts` parses**;
* `NodeHealthFlag` (HEALTH\_n flag bits) and `NodeFaultCode` (the compact wire faultId dictionary the client maps to labels);
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
    HOST_LINK_LOST = 0x05
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
#define NODE_STATUS_PROTO_VERSION `2`
```




<hr>



### define NODE\_STATUS\_REQ\_ADDR 

```C++
#define NODE_STATUS_REQ_ADDR `0x86FE`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/NodeStatus/NodeStatus.h`

