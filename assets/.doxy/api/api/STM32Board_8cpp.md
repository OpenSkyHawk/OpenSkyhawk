

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
|  bool | [**\_blinkPhase**](#variable-_blinkphase)   = `false`<br> |
|  bool | [**\_debugOn**](#variable-_debugon)   = `false`<br> |
|  CAN\_HandleTypeDef | [**\_hcan**](#variable-_hcan)  <br> |
|  uint32\_t | [**\_ledLastMs**](#variable-_ledlastms)   = `0`<br> |
|  [**LedState**](STM32Board_8cpp.md#enum-ledstate) | [**\_state**](#variable-_state)   = `LedState::OFF`<br> |














## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**HAL\_CAN\_MspInit**](#function-hal_can_mspinit) (CAN\_HandleTypeDef \* hcan\_p) <br> |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  void | [**\_applyLed**](#function-_applyled) () <br> |
|  uint16\_t | [**\_blinkPeriodFor**](#function-_blinkperiodfor) ([**LedState**](STM32Board_8cpp.md#enum-ledstate) s) <br> |
|  HardwareSerial | [**\_diag**](#function-_diag) (PA10, PA9) <br> |
|  void | [**\_setLedState**](#function-_setledstate) ([**LedState**](STM32Board_8cpp.md#enum-ledstate) s) <br> |


























## Public Types Documentation




### enum LedState 

```C++
enum LedState {
    OFF,
    BOOTING,
    NORMAL,
    CAN_ERROR,
    BUS_OFF,
    WARNING
};
```




<hr>
## Public Static Attributes Documentation




### variable \_blinkPhase 

```C++
bool _blinkPhase;
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



### variable \_state 

```C++
LedState _state;
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



### function \_setLedState 

```C++
static void _setLedState (
    LedState s
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/STM32Board/STM32Board.cpp`

