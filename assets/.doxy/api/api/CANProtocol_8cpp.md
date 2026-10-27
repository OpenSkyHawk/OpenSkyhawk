

# File CANProtocol.cpp



[**FileList**](files.md) **>** [**CANProtocol**](dir_81ff3032570f78b12938068450b63228.md) **>** [**CANProtocol.cpp**](CANProtocol_8cpp.md)

[Go to the source code of this file](CANProtocol_8cpp_source.md)



* `#include "CANProtocol.h"`
* `#include <STM32Board.h>`
* `#include <FirmwareVersion.h>`
* `#include <BootProtocol.h>`
* `#include <Arduino.h>`
* `#include <string.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**CANProtocol**](namespaceCANProtocol.md) <br> |


## Classes

| Type | Name |
| ---: | :--- |
| struct | [**BatchState**](structBatchState.md) <br> |
| struct | [**RxQueueEntry**](structRxQueueEntry.md) <br> |
| struct | [**TxQueueEntry**](structTxQueueEntry.md) <br> |








## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint32\_t | [**BATCH\_HOLD\_MS**](#variable-batch_hold_ms)   = `(NODE\_ID) == [**GATEWAY\_NODE\_ID**](NodeStatus_8h.md#variable-gateway_node_id) ? 2 : 0`<br> |
|  constexpr uint32\_t | [**HB\_PERIOD\_MS**](#variable-hb_period_ms)   = `500`<br> |
|  constexpr uint32\_t | [**HEALTH\_PERIOD\_MS**](#variable-health_period_ms)   = `1000`<br> |
|  constexpr uint8\_t | [**MAX\_FILTER\_IDS**](#variable-max_filter_ids)   = `16`<br> |
|  constexpr uint8\_t | [**MAX\_TX\_ATTEMPTS**](#variable-max_tx_attempts)   = `3`<br> |
|  constexpr uint8\_t | [**RX\_RING\_SIZE**](#variable-rx_ring_size)   = `((NODE\_ID) == [**GATEWAY\_NODE\_ID**](NodeStatus_8h.md#variable-gateway_node_id) ? 128 : 8)`<br> |
|  const uint32\_t | [**TSR\_ALST**](#variable-tsr_alst)   = `{ CAN\_TSR\_ALST0, CAN\_TSR\_ALST1, CAN\_TSR\_ALST2 }`<br> |
|  const uint32\_t | [**TSR\_RQCP**](#variable-tsr_rqcp)   = `{ CAN\_TSR\_RQCP0, CAN\_TSR\_RQCP1, CAN\_TSR\_RQCP2 }`<br> |
|  const uint32\_t | [**TSR\_TERR**](#variable-tsr_terr)   = `{ CAN\_TSR\_TERR0, CAN\_TSR\_TERR1, CAN\_TSR\_TERR2 }`<br> |
|  const uint32\_t | [**TSR\_TME**](#variable-tsr_tme)   = `{ CAN\_TSR\_TME0,  CAN\_TSR\_TME1,  CAN\_TSR\_TME2  }`<br> |
|  const uint32\_t | [**TSR\_TXOK**](#variable-tsr_txok)   = `{ CAN\_TSR\_TXOK0, CAN\_TSR\_TXOK1, CAN\_TSR\_TXOK2 }`<br> |
|  constexpr uint8\_t | [**TX\_RING\_SIZE**](#variable-tx_ring_size)   = `((NODE\_ID) == [**GATEWAY\_NODE\_ID**](NodeStatus_8h.md#variable-gateway_node_id) ? 64 : 16)`<br> |
|  [**BatchState**](structBatchState.md) | [**\_batches**](#variable-_batches)  <br> |
|  bool | [**\_dupSeen**](#variable-_dupseen)   = `false`<br> |
|  bool | [**\_everSawMaster**](#variable-_eversawmaster)   = `false`<br> |
|  uint8\_t | [**\_filterCount**](#variable-_filtercount)   = `0`<br> |
|  uint32\_t | [**\_filterIds**](#variable-_filterids)  <br> |
|  bool | [**\_filterPassAll**](#variable-_filterpassall)   = `false`<br> |
|  [**TxQueueEntry**](structTxQueueEntry.md) | [**\_inMailbox**](#variable-_inmailbox)  <br> |
|  uint32\_t | [**\_lastDupMs**](#variable-_lastdupms)   = `0`<br> |
|  uint32\_t | [**\_lastHbMs**](#variable-_lasthbms)   = `0`<br> |
|  NodeHealthPayload | [**\_lastHealth**](#variable-_lasthealth)   = `{ 0, [**DIE\_TEMP\_UNAVAILABLE**](NodeStatus_8h.md#variable-die_temp_unavailable), 0, 0, 0, 0, { 0, 0 } }`<br> |
|  uint32\_t | [**\_lastHealthMs**](#variable-_lasthealthms)   = `0`<br> |
|  uint32\_t | [**\_lastMasterMs**](#variable-_lastmasterms)   = `0`<br> |
|  bool | [**\_loopback**](#variable-_loopback)   = `false`<br> |
|  bool | [**\_masterHbPending**](#variable-_masterhbpending)   = `false`<br> |
|  bool | [**\_masterLost**](#variable-_masterlost)   = `false`<br> |
|  bool | [**\_overheat**](#variable-_overheat)   = `false`<br> |
|  uint8\_t | [**\_ownHbLoopPending**](#variable-_ownhblooppending)   = `0`<br> |
|  bool | [**\_readySent**](#variable-_readysent)   = `false`<br> |
|  bool | [**\_resyncPending**](#variable-_resyncpending)   = `false`<br> |
|  uint32\_t | [**\_resyncSinceMs**](#variable-_resyncsincems)   = `0`<br> |
|  [**CanRxCallback**](CANProtocol_8h.md#typedef-canrxcallback) | [**\_rxCb**](#variable-_rxcb)   = `nullptr`<br> |
|  uint32\_t | [**\_rxDrops**](#variable-_rxdrops)   = `0`<br> |
|  volatile uint8\_t | [**\_rxHead**](#variable-_rxhead)   = `0`<br> |
|  uint32\_t | [**\_rxOverruns**](#variable-_rxoverruns)   = `0`<br> |
|  [**RxQueueEntry**](structRxQueueEntry.md) | [**\_rxRing**](#variable-_rxring)  <br> |
|  volatile uint8\_t | [**\_rxTail**](#variable-_rxtail)   = `0`<br> |
|  [**CanStatus**](CANProtocol_8h.md#enum-canstatus) | [**\_status**](#variable-_status)   = `CanStatus::STARTING`<br> |
|  bool | [**\_statusArmed**](#variable-_statusarmed)   = `false`<br> |
|  [**CanStatusCallback**](CANProtocol_8h.md#typedef-canstatuscallback) | [**\_statusCb**](#variable-_statuscb)   = `nullptr`<br> |
|  [**CanSyncReqCallback**](CANProtocol_8h.md#typedef-cansyncreqcallback) | [**\_syncReqCb**](#variable-_syncreqcb)   = `nullptr`<br> |
|  uint32\_t | [**\_txDrops**](#variable-_txdrops)   = `0`<br> |
|  volatile uint8\_t | [**\_txHead**](#variable-_txhead)   = `0`<br> |
|  [**TxQueueEntry**](structTxQueueEntry.md) | [**\_txRing**](#variable-_txring)  <br> |
|  volatile uint8\_t | [**\_txTail**](#variable-_txtail)   = `0`<br> |
|  bool | [**\_warnAsserted**](#variable-_warnasserted)   = `false`<br> |














## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**USB\_HP\_CAN1\_TX\_IRQHandler**](#function-usb_hp_can1_tx_irqhandler) (void) <br> |
|  void | [**USB\_LP\_CAN1\_RX0\_IRQHandler**](#function-usb_lp_can1_rx0_irqhandler) (void) <br> |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  void | [**\_applyFilters**](#function-_applyfilters) () <br> |
|  void | [**\_drainTxQueue**](#function-_draintxqueue) () <br> |
|  void | [**\_drainTxQueueFromMain**](#function-_draintxqueuefrommain) () <br> |
|  void | [**\_enqueueRxFrame**](#function-_enqueuerxframe) (uint32\_t canId, uint8\_t len, const uint8\_t \* data) <br> |
|  void | [**\_pollRxFifo0**](#function-_pollrxfifo0) () <br> |
|  void | [**\_requeueFront**](#function-_requeuefront) (const [**TxQueueEntry**](structTxQueueEntry.md) & e) <br> |
|  void | [**\_settleMailbox**](#function-_settlemailbox) (int i, uint32\_t tsr) <br> |
|  void | [**\_startInternal**](#function-_startinternal) (uint32\_t mode) <br> |
|  void | [**\_updateStatus**](#function-_updatestatus) () <br> |

























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**CAN\_RX\_RING\_SIZE**](CANProtocol_8cpp.md#define-can_rx_ring_size)  `((NODE\_ID) == [**GATEWAY\_NODE\_ID**](NodeStatus_8h.md#variable-gateway_node_id) ? 128 : 8)`<br> |
| define  | [**CAN\_TX\_RING\_SIZE**](CANProtocol_8cpp.md#define-can_tx_ring_size)  `((NODE\_ID) == [**GATEWAY\_NODE\_ID**](NodeStatus_8h.md#variable-gateway_node_id) ? 64 : 16)`<br> |

## Public Static Attributes Documentation




### variable BATCH\_HOLD\_MS 

```C++
constexpr uint32_t BATCH_HOLD_MS;
```




<hr>



### variable HB\_PERIOD\_MS 

```C++
constexpr uint32_t HB_PERIOD_MS;
```




<hr>



### variable HEALTH\_PERIOD\_MS 

```C++
constexpr uint32_t HEALTH_PERIOD_MS;
```




<hr>



### variable MAX\_FILTER\_IDS 

```C++
constexpr uint8_t MAX_FILTER_IDS;
```




<hr>



### variable MAX\_TX\_ATTEMPTS 

```C++
constexpr uint8_t MAX_TX_ATTEMPTS;
```




<hr>



### variable RX\_RING\_SIZE 

```C++
constexpr uint8_t RX_RING_SIZE;
```




<hr>



### variable TSR\_ALST 

```C++
const uint32_t TSR_ALST[3];
```




<hr>



### variable TSR\_RQCP 

```C++
const uint32_t TSR_RQCP[3];
```




<hr>



### variable TSR\_TERR 

```C++
const uint32_t TSR_TERR[3];
```




<hr>



### variable TSR\_TME 

```C++
const uint32_t TSR_TME[3];
```




<hr>



### variable TSR\_TXOK 

```C++
const uint32_t TSR_TXOK[3];
```




<hr>



### variable TX\_RING\_SIZE 

```C++
constexpr uint8_t TX_RING_SIZE;
```




<hr>



### variable \_batches 

```C++
BatchState _batches[5];
```




<hr>



### variable \_dupSeen 

```C++
bool _dupSeen;
```




<hr>



### variable \_everSawMaster 

```C++
bool _everSawMaster;
```




<hr>



### variable \_filterCount 

```C++
uint8_t _filterCount;
```




<hr>



### variable \_filterIds 

```C++
uint32_t _filterIds[MAX_FILTER_IDS];
```




<hr>



### variable \_filterPassAll 

```C++
bool _filterPassAll;
```




<hr>



### variable \_inMailbox 

```C++
TxQueueEntry _inMailbox[3];
```




<hr>



### variable \_lastDupMs 

```C++
uint32_t _lastDupMs;
```




<hr>



### variable \_lastHbMs 

```C++
uint32_t _lastHbMs;
```




<hr>



### variable \_lastHealth 

```C++
NodeHealthPayload _lastHealth;
```




<hr>



### variable \_lastHealthMs 

```C++
uint32_t _lastHealthMs;
```




<hr>



### variable \_lastMasterMs 

```C++
uint32_t _lastMasterMs;
```




<hr>



### variable \_loopback 

```C++
bool _loopback;
```




<hr>



### variable \_masterHbPending 

```C++
bool _masterHbPending;
```




<hr>



### variable \_masterLost 

```C++
bool _masterLost;
```




<hr>



### variable \_overheat 

```C++
bool _overheat;
```




<hr>



### variable \_ownHbLoopPending 

```C++
uint8_t _ownHbLoopPending;
```




<hr>



### variable \_readySent 

```C++
bool _readySent;
```




<hr>



### variable \_resyncPending 

```C++
bool _resyncPending;
```




<hr>



### variable \_resyncSinceMs 

```C++
uint32_t _resyncSinceMs;
```




<hr>



### variable \_rxCb 

```C++
CanRxCallback _rxCb;
```




<hr>



### variable \_rxDrops 

```C++
uint32_t _rxDrops;
```




<hr>



### variable \_rxHead 

```C++
volatile uint8_t _rxHead;
```




<hr>



### variable \_rxOverruns 

```C++
uint32_t _rxOverruns;
```




<hr>



### variable \_rxRing 

```C++
RxQueueEntry _rxRing[RX_RING_SIZE];
```




<hr>



### variable \_rxTail 

```C++
volatile uint8_t _rxTail;
```




<hr>



### variable \_status 

```C++
CanStatus _status;
```




<hr>



### variable \_statusArmed 

```C++
bool _statusArmed;
```




<hr>



### variable \_statusCb 

```C++
CanStatusCallback _statusCb;
```




<hr>



### variable \_syncReqCb 

```C++
CanSyncReqCallback _syncReqCb;
```




<hr>



### variable \_txDrops 

```C++
uint32_t _txDrops;
```




<hr>



### variable \_txHead 

```C++
volatile uint8_t _txHead;
```




<hr>



### variable \_txRing 

```C++
TxQueueEntry _txRing[TX_RING_SIZE];
```




<hr>



### variable \_txTail 

```C++
volatile uint8_t _txTail;
```




<hr>



### variable \_warnAsserted 

```C++
bool _warnAsserted;
```




<hr>
## Public Functions Documentation




### function USB\_HP\_CAN1\_TX\_IRQHandler 

```C++
void USB_HP_CAN1_TX_IRQHandler (
    void
) 
```




<hr>



### function USB\_LP\_CAN1\_RX0\_IRQHandler 

```C++
void USB_LP_CAN1_RX0_IRQHandler (
    void
) 
```




<hr>
## Public Static Functions Documentation




### function \_applyFilters 

```C++
static void _applyFilters () 
```




<hr>



### function \_drainTxQueue 

```C++
static void _drainTxQueue () 
```




<hr>



### function \_drainTxQueueFromMain 

```C++
static void _drainTxQueueFromMain () 
```




<hr>



### function \_enqueueRxFrame 

```C++
static void _enqueueRxFrame (
    uint32_t canId,
    uint8_t len,
    const uint8_t * data
) 
```




<hr>



### function \_pollRxFifo0 

```C++
static void _pollRxFifo0 () 
```




<hr>



### function \_requeueFront 

```C++
static void _requeueFront (
    const TxQueueEntry & e
) 
```




<hr>



### function \_settleMailbox 

```C++
static void _settleMailbox (
    int i,
    uint32_t tsr
) 
```




<hr>



### function \_startInternal 

```C++
static void _startInternal (
    uint32_t mode
) 
```




<hr>



### function \_updateStatus 

```C++
static void _updateStatus () 
```




<hr>
## Macro Definition Documentation





### define CAN\_RX\_RING\_SIZE 

```C++
#define CAN_RX_RING_SIZE `((NODE_ID) == GATEWAY_NODE_ID ? 128 : 8)`
```




<hr>



### define CAN\_TX\_RING\_SIZE 

```C++
#define CAN_TX_RING_SIZE `((NODE_ID) == GATEWAY_NODE_ID ? 64 : 16)`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/CANProtocol/CANProtocol.cpp`

