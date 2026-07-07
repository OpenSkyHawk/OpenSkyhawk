

# File CANProtocol.h



[**FileList**](files.md) **>** [**CANProtocol**](dir_81ff3032570f78b12938068450b63228.md) **>** [**CANProtocol.h**](CANProtocol_8h.md)

[Go to the source code of this file](CANProtocol_8h_source.md)

_Shared CAN bus types, frame IDs, and runtime API for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _STM32 nodes._[More...](#detailed-description)

* `#include <stdint.h>`
* `#include <HIDControls.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**CANProtocol**](namespaceCANProtocol.md) <br> |




## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\*)(uint32\_t canId, const uint8\_t \*data, uint8\_t len) | [**CanRxCallback**](#typedef-canrxcallback)  <br>_Fired when a CAN frame is received. Register via onReceive()._  |
| enum  | [**CanStatus**](#enum-canstatus)  <br>_CAN bus health states. Reported to_ [_**STM32Board**_](namespaceSTM32Board.md) _via onStatusChange()._ |
| typedef void(\*)([**CanStatus**](CANProtocol_8h.md#enum-canstatus) status) | [**CanStatusCallback**](#typedef-canstatuscallback)  <br>_Fired when CAN bus status changes. Register via onStatusChange()._  |
| typedef void(\*)() | [**CanSyncReqCallback**](#typedef-cansyncreqcallback)  <br>_Fired when SYNC\_REQ is received. Register via onSyncReq()._  |






## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint32\_t | [**CAN\_ID\_CTRL\_BCAST**](#variable-can_id_ctrl_bcast)   = `0x010`<br>_Broadcast ControlPacketPair to all panels._  |
|  constexpr uint32\_t | [**CAN\_ID\_SYNC\_REQ**](#variable-can_id_sync_req)   = `0x012`<br>_Request all nodes to re-poll inputs._  |
|  constexpr uint32\_t | [**CAN\_ID\_TEST\_SEQ**](#variable-can_id_test_seq)   = `0x011`<br>_RTT throughput test._  |
|  constexpr uint16\_t | [**CTRL\_ID\_DCS\_MAX**](#variable-ctrl_id_dcs_max)   = `0x86FF`<br>_DCS-BIOS range end._  |
|  constexpr uint16\_t | [**CTRL\_ID\_DCS\_MIN**](#variable-ctrl_id_dcs_min)   = `0x8000`<br>_DCS-BIOS range start._  |
|  constexpr uint8\_t | [**DIAG\_ERR**](#variable-diag_err)   = `0x03`<br>_CAN error counter frame._  |
|  constexpr uint8\_t | [**DIAG\_EVT**](#variable-diag_evt)   = `0x04`<br>_Sub-node input event forwarded upstream._  |
|  constexpr uint8\_t | [**DIAG\_HB**](#variable-diag_hb)   = `0x02`<br>_Sub-node heartbeat frame._  |
|  constexpr uint8\_t | [**DIAG\_MAGIC**](#variable-diag_magic)   = `0xAA`<br>_Frame sync byte._  |
|  constexpr uint8\_t | [**DIAG\_RTT**](#variable-diag_rtt)   = `0x01`<br>_Round-trip time measurement frame._  |














## Public Functions

| Type | Name |
| ---: | :--- |
|  struct | [**\_\_attribute\_\_**](#function-__attribute__) ((packed)) <br>_Primary input/output routing packet. 4 bytes; two are batched for CTRL\_BCAST/EVT\_n._  |
|  constexpr uint32\_t | [**canIdEcho**](#function-canidecho) (uint8\_t n) <br>_TEST\_SEQ echo frame ID for node n. Range 0x301-0x33F._  |
|  constexpr uint32\_t | [**canIdEvt**](#function-canidevt) (uint8\_t n) <br>_Input event frame ID for node n. Range 0x201-0x23F._  |
|  constexpr uint32\_t | [**canIdEvtDir**](#function-canidevtdir) (uint8\_t n) <br>_Directional-input event frame ID for node n. RotaryEncoder DIR mode: payload value is a signed ±1 (int16); the bridge formats it_ `INC` _/_`DEC` _for a DCS-BIOS fixed\_step control. Range 0x601-0x63F._ |
|  constexpr uint32\_t | [**canIdEvtRel**](#function-canidevtrel) (uint8\_t n) <br>_Relative-input event frame ID for node n. RotaryEncoder REL mode: payload value is a signed ±step (int16); the bridge formats it_ `%+d` _for a DCS-BIOS variable\_step control. Range 0x501-0x53F._ |
|  constexpr uint32\_t | [**canIdHb**](#function-canidhb) (uint8\_t n) <br>_Heartbeat frame ID for node n. Range 0x100-0x13F; n=0 is_ [_**PanelBridge**_](namespacePanelBridge.md) _._ |
|  constexpr uint32\_t | [**canIdHealth**](#function-canidhealth) (uint8\_t n) <br>_Node-health/thermal telemetry frame ID for node n. Range 0x140-0x17F; n=0 is_ [_**PanelBridge**_](namespacePanelBridge.md) _. Carries NodeHealthPayload (internal die temp; degraded/faultId once #163 lands)._ |
|  constexpr uint32\_t | [**canIdReady**](#function-canidready) (uint8\_t n) <br>_Boot-complete READY frame ID for node n. Range 0x401-0x43F._  |




























## Detailed Description


Owns all CAN bus interaction for [**PanelGroup**](namespacePanelGroup.md) and [**PanelBridge**](namespacePanelBridge.md) nodes. Types and constants (ControlPacket, CanStatus, frame IDs, CAN ID functions) are platform-agnostic. The runtime namespace (filters, lifecycle, send, callbacks, diagnostics) is STM32-only and guarded by ARDUINO\_ARCH\_STM32. CAN arbitration IDs (CAN\_ID\_\*, canId\*()) and payload ControlPacket::controlId values are separate namespaces; equal numeric values do not conflict because they occupy different CAN frame fields.


Dependency: [**STM32Board::begin()**](namespaceSTM32Board.md#function-begin) must be called before [**CANProtocol::start()**](namespaceCANProtocol.md#function-start). [**CANProtocol**](namespaceCANProtocol.md) owns CAN bus operation; [**STM32Board**](namespaceSTM32Board.md) owns peripheral hardware init.




**Version:**

0.3.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    
## Public Types Documentation




### typedef CanRxCallback 

_Fired when a CAN frame is received. Register via onReceive()._ 
```C++
using CanRxCallback =  void(*)(uint32_t canId, const uint8_t* data, uint8_t len);
```




<hr>



### enum CanStatus 

_CAN bus health states. Reported to_ [_**STM32Board**_](namespaceSTM32Board.md) _via onStatusChange()._
```C++
enum CanStatus {
    STARTING,
    NORMAL,
    TX_ERROR,
    BUS_OFF
};
```




<hr>



### typedef CanStatusCallback 

_Fired when CAN bus status changes. Register via onStatusChange()._ 
```C++
using CanStatusCallback =  void(*)(CanStatus status);
```




<hr>



### typedef CanSyncReqCallback 

_Fired when SYNC\_REQ is received. Register via onSyncReq()._ 
```C++
using CanSyncReqCallback =  void(*)();
```




<hr>
## Public Static Attributes Documentation




### variable CAN\_ID\_CTRL\_BCAST 

_Broadcast ControlPacketPair to all panels._ 
```C++
constexpr uint32_t CAN_ID_CTRL_BCAST;
```




<hr>



### variable CAN\_ID\_SYNC\_REQ 

_Request all nodes to re-poll inputs._ 
```C++
constexpr uint32_t CAN_ID_SYNC_REQ;
```




<hr>



### variable CAN\_ID\_TEST\_SEQ 

_RTT throughput test._ 
```C++
constexpr uint32_t CAN_ID_TEST_SEQ;
```




<hr>



### variable CTRL\_ID\_DCS\_MAX 

_DCS-BIOS range end._ 
```C++
constexpr uint16_t CTRL_ID_DCS_MAX;
```




<hr>



### variable CTRL\_ID\_DCS\_MIN 

_DCS-BIOS range start._ 
```C++
constexpr uint16_t CTRL_ID_DCS_MIN;
```




<hr>



### variable DIAG\_ERR 

_CAN error counter frame._ 
```C++
constexpr uint8_t DIAG_ERR;
```




<hr>



### variable DIAG\_EVT 

_Sub-node input event forwarded upstream._ 
```C++
constexpr uint8_t DIAG_EVT;
```




<hr>



### variable DIAG\_HB 

_Sub-node heartbeat frame._ 
```C++
constexpr uint8_t DIAG_HB;
```




<hr>



### variable DIAG\_MAGIC 

_Frame sync byte._ 
```C++
constexpr uint8_t DIAG_MAGIC;
```




<hr>



### variable DIAG\_RTT 

_Round-trip time measurement frame._ 
```C++
constexpr uint8_t DIAG_RTT;
```




<hr>
## Public Functions Documentation




### function \_\_attribute\_\_ 

_Primary input/output routing packet. 4 bytes; two are batched for CTRL\_BCAST/EVT\_n._ 
```C++
struct __attribute__ (
    (packed)
) 
```



8-byte payload carried by HEALTH\_n frames — per-node health telemetry.


8-byte payload carried by HB\_n heartbeat frames.


Two ControlPackets packed into one 8-byte input/output CAN frame.


Used by CTRL\_BCAST and EVT\_n only. Slot B controlId == 0x0000 signals an empty/padding slot.


Sent every 500 ms by [**PanelGroup**](namespacePanelGroup.md) nodes. [**PanelBridge**](namespacePanelBridge.md) reads HB\_1–HB\_63 to track [**PanelGroup**](namespacePanelGroup.md) health and populate diagnostics. HB\_0 is reserved but not transmitted.


Sent every 1000 ms by every STM32 node. This is the shared node-health wire contract; separate features populate their own fields, all within the fixed 8 bytes:
* Temperature (#213): dieTempC, flags bit0 — read from the MCU's built-in internal temperature sensor (ADC ch16); no external parts, no PCB change.
* Degraded state (#163): flags bit1, faultMask, faultId — a node that is alive but has a faulted FaultSource reporting a NodeFaultCode ([**NodeStatus.h**](NodeStatus_8h.md)). Transmit 0 until that lands. [**PanelBridge**](namespacePanelBridge.md) caches HEALTH\_1–HEALTH\_63 per node and forwards them in \_NODE\_STATUS.






**Note:**

The STM32F103 internal sensor is UNCALIBRATED (no factory trim): ~±few °C absolute, measures DIE temperature (not ambient) with a self-heat offset. Use for relative trend and per-node overheat flagging, not precise ambient measurement. 




**Note:**

Rail voltage/current is intentionally NOT here — that is the PDU's dedicated power telemetry (#202, real INA226 sensors), not generic per-node health. 





        

<hr>



### function canIdEcho 

_TEST\_SEQ echo frame ID for node n. Range 0x301-0x33F._ 
```C++
constexpr uint32_t canIdEcho (
    uint8_t n
) 
```




<hr>



### function canIdEvt 

_Input event frame ID for node n. Range 0x201-0x23F._ 
```C++
constexpr uint32_t canIdEvt (
    uint8_t n
) 
```




<hr>



### function canIdEvtDir 

_Directional-input event frame ID for node n. RotaryEncoder DIR mode: payload value is a signed ±1 (int16); the bridge formats it_ `INC` _/_`DEC` _for a DCS-BIOS fixed\_step control. Range 0x601-0x63F._
```C++
constexpr uint32_t canIdEvtDir (
    uint8_t n
) 
```




<hr>



### function canIdEvtRel 

_Relative-input event frame ID for node n. RotaryEncoder REL mode: payload value is a signed ±step (int16); the bridge formats it_ `%+d` _for a DCS-BIOS variable\_step control. Range 0x501-0x53F._
```C++
constexpr uint32_t canIdEvtRel (
    uint8_t n
) 
```




<hr>



### function canIdHb 

_Heartbeat frame ID for node n. Range 0x100-0x13F; n=0 is_ [_**PanelBridge**_](namespacePanelBridge.md) _._
```C++
constexpr uint32_t canIdHb (
    uint8_t n
) 
```




<hr>



### function canIdHealth 

_Node-health/thermal telemetry frame ID for node n. Range 0x140-0x17F; n=0 is_ [_**PanelBridge**_](namespacePanelBridge.md) _. Carries NodeHealthPayload (internal die temp; degraded/faultId once #163 lands)._
```C++
constexpr uint32_t canIdHealth (
    uint8_t n
) 
```




<hr>



### function canIdReady 

_Boot-complete READY frame ID for node n. Range 0x401-0x43F._ 
```C++
constexpr uint32_t canIdReady (
    uint8_t n
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/CANProtocol/CANProtocol.h`

