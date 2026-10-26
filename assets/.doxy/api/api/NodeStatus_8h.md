

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
| enum uint8\_t | [**HeartbeatFlag**](#enum-heartbeatflag)  <br>_HB\_n_ `flags` _bits — CAN controller error state (maps to HeartbeatPayload.flags)._ |
| enum uint8\_t | [**NodeFaultCode**](#enum-nodefaultcode)  <br>_HEALTH\_n_ `faultId` _dictionary (#163) — cross-node fault codes._ |
| enum uint8\_t | [**NodeHealthFlag**](#enum-nodehealthflag)  <br>_HEALTH\_n_ `flags` _bits — node-level health conditions (maps to NodeHealthPayload.flags)._ |
| enum uint8\_t | [**NodeKind**](#enum-nodekind)  <br>_HEALTH\_n_ `nodeKind` _(byte 5) — what kind of board this node is (#344)._ |
| enum uint8\_t | [**NodeStatusField**](#enum-nodestatusfield)  <br>`_NODE_STATUS` _row field offsets, in bytes of the decoded row (hex index = 2 × offset)._ |
| enum uint8\_t | [**NodeTelemField**](#enum-nodetelemfield)  <br>`_NODE_TELEM` _row field offsets, in bytes of the decoded row (hex index = 2 × offset)._ |
| enum uint8\_t | [**PduRailField**](#enum-pdurailfield)  <br>_PDU\_RAIL\_12V / PDU\_RAIL\_5V field offsets, in bytes of the 8-byte TELEM\_n frame._  |
| enum uint8\_t | [**PduRailFlag**](#enum-pdurailflag)  <br>_PDU\_RAIL\_\* page byte 1 — per-rail condition bits (all zero = healthy)._  |
| enum uint8\_t | [**PduStatusField**](#enum-pdustatusfield)  <br>_PDU\_STATUS field offsets, in bytes of the 8-byte TELEM\_n frame._  |
| enum uint8\_t | [**PduStatusFlag**](#enum-pdustatusflag)  <br>_PDU\_STATUS page byte 1 — board condition bits (all zero = healthy, calibrated)._  |
| enum uint8\_t | [**TelemPage**](#enum-telempage)  <br>_TELEM\_n page ids (#216) — byte 0 of TelemPayload, naming the layout of bytes 1-7._  |




## Public Attributes

| Type | Name |
| ---: | :--- |
|  constexpr int8\_t | [**DIE\_TEMP\_UNAVAILABLE**](#variable-die_temp_unavailable)   = `-128`<br>_HEALTH\_n_ `dieTempC` _meaning "unavailable / not yet seen" (_`80` _on the wire)._ |
|  constexpr uint8\_t | [**GATEWAY\_NODE\_ID**](#variable-gateway_node_id)   = `0`<br>_Node ID of the Gateway board's_ [_**PanelBridge**_](namespacePanelBridge.md) _MCU — the CAN master._ |
|  constexpr uint8\_t | [**HB\_ESR\_REC\_SHIFT**](#variable-hb_esr_rec_shift)   = `8`<br>_HB\_n_ `esr` _field: bit position of the receive error counter (REC, high byte)._ |
|  constexpr uint8\_t | [**HB\_ESR\_TEC\_SHIFT**](#variable-hb_esr_tec_shift)   = `0`<br>_HB\_n_ `esr` _field: bit position of the transmit error counter (TEC, low byte)._ |
|  constexpr uint8\_t | [**NODE\_BOOT\_STATE\_APP**](#variable-node_boot_state_app)   = `0x00`<br>`BOOT_STATE` _values (#365): the app is running; any other value is the bootloader's BootState._ |
|  constexpr uint8\_t | [**NODE\_BOOT\_STATE\_UPDATING**](#variable-node_boot_state_updating)   = `0x80`<br> |
|  constexpr uint16\_t | [**NODE\_BOOT\_UPDATING\_MS**](#variable-node_boot_updating_ms)   = `3000`<br> |
|  constexpr uint8\_t | [**NODE\_STATUS\_ROW\_BYTES**](#variable-node_status_row_bytes)   = `19`<br> |
|  constexpr uint8\_t | [**NODE\_TELEM\_ROW\_BYTES**](#variable-node_telem_row_bytes)   = `9`<br>`_NODE_TELEM` _row length in bytes, sent as twice as many hex chars (9 → 18 hex)._ |
|  constexpr int16\_t | [**NTC\_TEMP\_UNAVAILABLE**](#variable-ntc_temp_unavailable)   = `-32768`<br>_PDU\_STATUS_ `ntcTempDC` _meaning "unavailable" — no reading yet, or the NTC is open/shorted._ |











































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**NODE\_STATUS\_END\_MSG\_NAME**](NodeStatus_8h.md#define-node_status_end_msg_name)  `"\_NODE\_STATUS\_END"`<br> |
| define  | [**NODE\_STATUS\_MSG\_NAME**](NodeStatus_8h.md#define-node_status_msg_name)  `"\_NODE\_STATUS"`<br> |
| define  | [**NODE\_STATUS\_PROTO\_VERSION**](NodeStatus_8h.md#define-node_status_proto_version)  `4`<br> |
| define  | [**NODE\_STATUS\_REQ\_ADDR**](NodeStatus_8h.md#define-node_status_req_addr)  `0xFF00`<br> |
| define  | [**NODE\_TELEM\_MSG\_NAME**](NodeStatus_8h.md#define-node_telem_msg_name)  `"\_NODE\_TELEM"`<br> |
| define  | [**NODE\_TELEM\_PROTO\_VERSION**](NodeStatus_8h.md#define-node_telem_proto_version)  `1`<br> |

## Detailed Description


Owns the cross-node **status API** — not just health:
* the `_NODE_STATUS` DCS-BIOS host-reporting contract (proto version, request address, message names, `GATEWAY_NODE_ID`, row layout `NodeStatusField`) — **the canonical source the client's `sync-a4ec.ts` parses**;
* `HeartbeatFlag` + the `HB_ESR_*` split (HB\_n flags / esr fields);
* `NodeHealthFlag` (HEALTH\_n flag bits), `DIE_TEMP_UNAVAILABLE`, and `NodeFaultCode` (the compact wire faultId dictionary the client maps to labels);
* `NodeKind` (HEALTH\_n board-kind byte — what board a NODE\_ID is, announced by its firmware);
* the `_NODE_TELEM` host contract and the TELEM\_n page vocabulary (`TelemPage`, `PduRailFlag`, `PduStatusFlag`, the per-page field offsets, `NTC_TEMP_UNAVAILABLE`) — #216;




Every wire value is a named `enum class X : T { NAME = literal }` or `constexpr T NAME = literal` so the client sync can read it (#346) — never only a comment, a bare literal or a format string.
* `FaultSource` — the interface a fault-producing object implements so a node-level aggregator can roll it up. A `FaultSource` is _one piece_ of node status, not the whole story.




This is the neutral home for node-status vocabulary so it isn't shaped around any one node flavour (PanelGroup/OutputBase) or the HID namespace. `NodeHealthPayload` (the CAN HEALTH\_n frame struct) stays in `CANProtocol.h`; the fault _detail_ strings stay local (DiagSerial), never on the wire. Header-only contract + a tiny `FaultSource` registry (`NodeStatus.cpp`).




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    
## Public Types Documentation




### enum HeartbeatFlag 

_HB\_n_ `flags` _bits — CAN controller error state (maps to HeartbeatPayload.flags)._
```C++
enum HeartbeatFlag {
    BOFF = 0x01,
    EPVF = 0x02
};
```




<hr>



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



### enum NodeStatusField 

`_NODE_STATUS` _row field offsets, in bytes of the decoded row (hex index = 2 × offset)._
```C++
enum NodeStatusField {
    ID = 0,
    PRESENT = 1,
    HB_FLAGS = 2,
    UPTIME = 3,
    RX_COUNT = 5,
    ESR = 7,
    DIE_TEMP_C = 9,
    HEALTH_FLAGS = 10,
    FAULT_MASK = 11,
    FAULT_ID = 12,
    NODE_KIND = 13,
    FW_MAJOR = 14,
    FW_MINOR = 15,
    FW_PATCH = 16,
    BOOTLOADER = 17,
    BOOT_STATE = 18
};
```



Each field is its numeric value as fixed-width uppercase hex, most-significant byte first, so the u16 fields read big-endian here although they are little-endian on CAN. A field runs to the next offset; the last runs to NODE\_STATUS\_ROW\_BYTES. The first six come from the node's HB\_n (plus `present`), the next five from its HEALTH\_n, the next three from its READY\_n (v4, #295), the last two its bootloader state (#365). Fields are only ever appended: a reader parses the fields it knows from a longer row and ignores the rest. (Not `NODE_ID`: every node build defines that as a macro.) 


        

<hr>



### enum NodeTelemField 

`_NODE_TELEM` _row field offsets, in bytes of the decoded row (hex index = 2 × offset)._
```C++
enum NodeTelemField {
    ID = 0,
    FRAME = 1
};
```




<hr>



### enum PduRailField 

_PDU\_RAIL\_12V / PDU\_RAIL\_5V field offsets, in bytes of the 8-byte TELEM\_n frame._ 
```C++
enum PduRailField {
    FLAGS = 1,
    V_AVG_MV = 2,
    V_MIN_MV = 4,
    I_AVG_MA = 6
};
```



Byte 0 is the TelemPage; a field runs to the next offset, the last to byte 7. Little-endian. 


        

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



### enum PduStatusField 

_PDU\_STATUS field offsets, in bytes of the 8-byte TELEM\_n frame._ 
```C++
enum PduStatusField {
    FLAGS = 1,
    NTC_TEMP_DC = 2,
    I12_PEAK_MA = 4,
    I5_PEAK_MA = 6
};
```



Byte 0 is the TelemPage; a field runs to the next offset, the last to byte 7. Little-endian. 


        

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



Global and append-only: a page id means the same layout whichever node sends it, so the client decodes by page, not by node type. 0x00 is invalid. Each page's layout is its field-offset enum below (PDU\_RAIL\_\* → PduRailField, PDU\_STATUS → PduStatusField); multi-byte fields little-endian. 


        

<hr>
## Public Attributes Documentation




### variable DIE\_TEMP\_UNAVAILABLE 

_HEALTH\_n_ `dieTempC` _meaning "unavailable / not yet seen" (_`80` _on the wire)._
```C++
constexpr int8_t DIE_TEMP_UNAVAILABLE;
```



Sent when the die sensor can't be read, and cached by [**PanelBridge**](namespacePanelBridge.md) for a node whose first HEALTH\_n hasn't arrived. Written as a literal so the client sync can read it; equal to INT8\_MIN. 


        

<hr>



### variable GATEWAY\_NODE\_ID 

_Node ID of the Gateway board's_ [_**PanelBridge**_](namespacePanelBridge.md) _MCU — the CAN master._
```C++
constexpr uint8_t GATEWAY_NODE_ID;
```



It sends HB\_0 / HEALTH\_0, and its own row leads every `_NODE_STATUS` roster reply (#344). CAN nodes take 1–63 (Firmware/NODE\_IDS.md). 


        

<hr>



### variable HB\_ESR\_REC\_SHIFT 

_HB\_n_ `esr` _field: bit position of the receive error counter (REC, high byte)._
```C++
constexpr uint8_t HB_ESR_REC_SHIFT;
```




<hr>



### variable HB\_ESR\_TEC\_SHIFT 

_HB\_n_ `esr` _field: bit position of the transmit error counter (TEC, low byte)._
```C++
constexpr uint8_t HB_ESR_TEC_SHIFT;
```




<hr>



### variable NODE\_BOOT\_STATE\_APP 

`BOOT_STATE` _values (#365): the app is running; any other value is the bootloader's BootState._
```C++
constexpr uint8_t NODE_BOOT_STATE_APP;
```




<hr>



### variable NODE\_BOOT\_STATE\_UPDATING 

```C++
constexpr uint8_t NODE_BOOT_STATE_UPDATING;
```



`BOOT_STATE` bit: update traffic for this node in the last NODE\_BOOT\_UPDATING\_MS. A board in its bootloader without it is waiting — "stuck" when its state is APP\_INVALID or APP\_UNSTABLE. 


        

<hr>



### variable NODE\_BOOT\_UPDATING\_MS 

```C++
constexpr uint16_t NODE_BOOT_UPDATING_MS;
```




<hr>



### variable NODE\_STATUS\_ROW\_BYTES 

```C++
constexpr uint8_t NODE_STATUS_ROW_BYTES;
```



`_NODE_STATUS` row length in bytes, sent as twice as many hex chars (proto v4: 19 → 38 hex; the two bootloader fields joined v4 before it shipped, #365). 


        

<hr>



### variable NODE\_TELEM\_ROW\_BYTES 

`_NODE_TELEM` _row length in bytes, sent as twice as many hex chars (9 → 18 hex)._
```C++
constexpr uint8_t NODE_TELEM_ROW_BYTES;
```




<hr>



### variable NTC\_TEMP\_UNAVAILABLE 

_PDU\_STATUS_ `ntcTempDC` _meaning "unavailable" — no reading yet, or the NTC is open/shorted._
```C++
constexpr int16_t NTC_TEMP_UNAVAILABLE;
```



`0x8000` on the wire. Written as a literal so the client sync can read it; equal to INT16\_MIN. 


        

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
#define NODE_STATUS_PROTO_VERSION `4`
```




<hr>



### define NODE\_STATUS\_REQ\_ADDR 

```C++
#define NODE_STATUS_REQ_ADDR `0xFF00`
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

