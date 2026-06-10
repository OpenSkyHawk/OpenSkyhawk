

# File CANProtocol.cpp



[**FileList**](files.md) **>** [**CANProtocol**](dir_81ff3032570f78b12938068450b63228.md) **>** [**CANProtocol.cpp**](CANProtocol_8cpp.md)

[Go to the source code of this file](CANProtocol_8cpp_source.md)



* `#include "CANProtocol.h"`
* `#include <STM32Board.h>`
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
|  constexpr uint8\_t | [**MAX\_FILTER\_IDS**](#variable-max_filter_ids)   = `16`<br> |
|  constexpr uint8\_t | [**MAX\_TX\_ATTEMPTS**](#variable-max_tx_attempts)   = `3`<br> |
|  constexpr uint8\_t | [**RX\_RING\_SIZE**](#variable-rx_ring_size)   = `8`<br> |
|  constexpr uint8\_t | [**TX\_RING\_SIZE**](#variable-tx_ring_size)   = `16`<br> |
|  [**BatchState**](structBatchState.md) | [**\_batches**](#variable-_batches)  <br> |
|  uint8\_t | [**\_filterCount**](#variable-_filtercount)   = `0`<br> |
|  uint32\_t | [**\_filterIds**](#variable-_filterids)  <br> |
|  bool | [**\_filterPassAll**](#variable-_filterpassall)   = `false`<br> |
|  [**CanRxCallback**](CANProtocol_8h.md#typedef-canrxcallback) | [**\_rxCb**](#variable-_rxcb)   = `nullptr`<br> |
|  volatile uint8\_t | [**\_rxHead**](#variable-_rxhead)   = `0`<br> |
|  [**RxQueueEntry**](structRxQueueEntry.md) | [**\_rxRing**](#variable-_rxring)  <br> |
|  volatile uint8\_t | [**\_rxTail**](#variable-_rxtail)   = `0`<br> |
|  [**CanStatus**](CANProtocol_8h.md#enum-canstatus) | [**\_status**](#variable-_status)   = `CanStatus::STARTING`<br> |
|  [**CanStatusCallback**](CANProtocol_8h.md#typedef-canstatuscallback) | [**\_statusCb**](#variable-_statuscb)   = `nullptr`<br> |
|  [**CanSyncReqCallback**](CANProtocol_8h.md#typedef-cansyncreqcallback) | [**\_syncReqCb**](#variable-_syncreqcb)   = `nullptr`<br> |
|  uint32\_t | [**\_txDrops**](#variable-_txdrops)   = `0`<br> |
|  volatile uint8\_t | [**\_txHead**](#variable-_txhead)   = `0`<br> |
|  [**TxQueueEntry**](structTxQueueEntry.md) | [**\_txRing**](#variable-_txring)  <br> |
|  volatile uint8\_t | [**\_txTail**](#variable-_txtail)   = `0`<br> |














## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**HAL\_CAN\_RxFifo0MsgPendingCallback**](#function-hal_can_rxfifo0msgpendingcallback) (CAN\_HandleTypeDef \* hcan) <br> |
|  void | [**HAL\_CAN\_TxMailbox0CompleteCallback**](#function-hal_can_txmailbox0completecallback) (CAN\_HandleTypeDef \*) <br> |
|  void | [**HAL\_CAN\_TxMailbox1CompleteCallback**](#function-hal_can_txmailbox1completecallback) (CAN\_HandleTypeDef \*) <br> |
|  void | [**HAL\_CAN\_TxMailbox2CompleteCallback**](#function-hal_can_txmailbox2completecallback) (CAN\_HandleTypeDef \*) <br> |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  void | [**\_applyFilters**](#function-_applyfilters) () <br> |
|  void | [**\_drainTxQueue**](#function-_draintxqueue) () <br> |
|  void | [**\_startInternal**](#function-_startinternal) (uint32\_t mode) <br> |
|  void | [**\_updateStatus**](#function-_updatestatus) () <br> |


























## Public Static Attributes Documentation




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



### variable TX\_RING\_SIZE 

```C++
constexpr uint8_t TX_RING_SIZE;
```




<hr>



### variable \_batches 

```C++
BatchState _batches[2];
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



### variable \_rxCb 

```C++
CanRxCallback _rxCb;
```




<hr>



### variable \_rxHead 

```C++
volatile uint8_t _rxHead;
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
## Public Functions Documentation




### function HAL\_CAN\_RxFifo0MsgPendingCallback 

```C++
void HAL_CAN_RxFifo0MsgPendingCallback (
    CAN_HandleTypeDef * hcan
) 
```




<hr>



### function HAL\_CAN\_TxMailbox0CompleteCallback 

```C++
void HAL_CAN_TxMailbox0CompleteCallback (
    CAN_HandleTypeDef *
) 
```




<hr>



### function HAL\_CAN\_TxMailbox1CompleteCallback 

```C++
void HAL_CAN_TxMailbox1CompleteCallback (
    CAN_HandleTypeDef *
) 
```




<hr>



### function HAL\_CAN\_TxMailbox2CompleteCallback 

```C++
void HAL_CAN_TxMailbox2CompleteCallback (
    CAN_HandleTypeDef *
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

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/CANProtocol/CANProtocol.cpp`

