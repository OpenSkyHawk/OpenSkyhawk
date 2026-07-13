

# File STM32Board.cpp



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**STM32Board**](dir_aa1816754c0645981f9c7af905857f7d.md) **>** [**STM32Board.cpp**](STM32Board_8cpp.md)

[Go to the source code of this file](STM32Board_8cpp_source.md)



* `#include "STM32Board.h"`
* `#include <CANProtocol.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**STM32Board**](namespaceSTM32Board.md) <br> |




## Public Types

| Type | Name |
| ---: | :--- |
| enum  | [**LedState**](#enum-ledstate)  <br> |






## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint32\_t | [**LINK\_DECAY\_MS**](#variable-link_decay_ms)   = `500`<br>_CONNECTED → NORMAL after this idle gap._  |
|  bool | [**\_begun**](#variable-_begun)   = `false`<br>_false → OFF (pre-begin)_  |
|  bool | [**\_blinkPhase**](#variable-_blinkphase)   = `false`<br> |
|  [**CanStatus**](CANProtocol_8h.md#enum-canstatus) | [**\_canStatus**](#variable-_canstatus)   = `CanStatus::STARTING`<br>_Last value from onCanStatus()_  |
|  bool | [**\_clockFault**](#variable-_clockfault)   = `false`<br>_SystemClock\_Config off HSE / wrong freq (issue #245)_  |
|  bool | [**\_debugOn**](#variable-_debugon)   = `false`<br> |
|  CAN\_HandleTypeDef | [**\_hcan**](#variable-_hcan)  <br> |
|  uint32\_t | [**\_ledLastMs**](#variable-_ledlastms)   = `0`<br> |
|  bool | [**\_linkActive**](#variable-_linkactive)   = `false`<br>_Data flowing (set by setLinkActive)_  |
|  uint32\_t | [**\_linkLastMs**](#variable-_linklastms)   = `0`<br>_millis() of last setLinkActive(true)_  |
|  [**LedState**](STM32Board_8cpp.md#enum-ledstate) | [**\_state**](#variable-_state)   = `LedState::OFF`<br> |
|  bool | [**\_warning**](#variable-_warning)   = `false`<br>_App-layer warning latch._  |














## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**HAL\_CAN\_MspInit**](#function-hal_can_mspinit) (CAN\_HandleTypeDef \* hcan\_p) <br> |
|  void | [**SystemClock\_Config**](#function-systemclock_config) (void) <br> |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  void | [**\_applyLed**](#function-_applyled) () <br> |
|  uint16\_t | [**\_blinkPeriodFor**](#function-_blinkperiodfor) ([**LedState**](STM32Board_8cpp.md#enum-ledstate) s) <br> |
|  HardwareSerial | [**\_diag**](#function-_diag) (PA10, PA9) <br> |
|  void | [**\_recompute**](#function-_recompute) () <br> |


























## Public Types Documentation




### enum LedState 

```C++
enum LedState {
    OFF,
    BOOTING,
    NORMAL,
    CONNECTED,
    CAN_ERROR,
    BUS_OFF,
    WARNING
};
```




<hr>
## Public Static Attributes Documentation




### variable LINK\_DECAY\_MS 

_CONNECTED → NORMAL after this idle gap._ 
```C++
constexpr uint32_t LINK_DECAY_MS;
```




<hr>



### variable \_begun 

_false → OFF (pre-begin)_ 
```C++
bool _begun;
```




<hr>



### variable \_blinkPhase 

```C++
bool _blinkPhase;
```




<hr>



### variable \_canStatus 

_Last value from onCanStatus()_ 
```C++
CanStatus _canStatus;
```




<hr>



### variable \_clockFault 

_SystemClock\_Config off HSE / wrong freq (issue #245)_ 
```C++
bool _clockFault;
```




<hr>



### variable \_debugOn 

```C++
bool _debugOn;
```




<hr>



### variable \_hcan 

```C++
CAN_HandleTypeDef _hcan;
```




<hr>



### variable \_ledLastMs 

```C++
uint32_t _ledLastMs;
```




<hr>



### variable \_linkActive 

_Data flowing (set by setLinkActive)_ 
```C++
bool _linkActive;
```




<hr>



### variable \_linkLastMs 

_millis() of last setLinkActive(true)_ 
```C++
uint32_t _linkLastMs;
```




<hr>



### variable \_state 

```C++
LedState _state;
```




<hr>



### variable \_warning 

_App-layer warning latch._ 
```C++
bool _warning;
```




<hr>
## Public Functions Documentation




### function HAL\_CAN\_MspInit 

```C++
void HAL_CAN_MspInit (
    CAN_HandleTypeDef * hcan_p
) 
```




<hr>



### function SystemClock\_Config 

```C++
void SystemClock_Config (
    void
) 
```




<hr>
## Public Static Functions Documentation




### function \_applyLed 

```C++
static void _applyLed () 
```




<hr>



### function \_blinkPeriodFor 

```C++
static uint16_t _blinkPeriodFor (
    LedState s
) 
```




<hr>



### function \_diag 

```C++
static HardwareSerial _diag (
    PA10,
    PA9
) 
```




<hr>



### function \_recompute 

```C++
static void _recompute () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/STM32Board/STM32Board.cpp`

