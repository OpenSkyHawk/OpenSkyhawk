

# File ShiftBus.cpp



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Helpers**](dir_9e93d9a1721bcf27b2030ff612e0fc11.md) **>** [**ShiftBus**](dir_5de82edf055e68e6d2d76fc20b67149e.md) **>** [**ShiftBus.cpp**](ShiftBus_8cpp.md)

[Go to the source code of this file](ShiftBus_8cpp_source.md)

[More...](#detailed-description)

* `#include "ShiftBus.h"`
* `#include <STM32Board.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |








## Public Attributes

| Type | Name |
| ---: | :--- |
|  [**OpenSkyhawk::ShiftBus**](classOpenSkyhawk_1_1ShiftBus.md) | [**ShiftBus1**](#variable-shiftbus1)  <br> |


















## Public Static Functions

| Type | Name |
| ---: | :--- |
|  const SPISettings | [**kShiftBusSettings**](#function-kshiftbussettings) (1000000, MSBFIRST, SPI\_MODE0) <br> |

























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SHIFTBUS\_LATCH**](ShiftBus_8cpp.md#define-shiftbus_latch)  `PB9`<br> |
| define  | [**SHIFTBUS\_LOAD**](ShiftBus_8cpp.md#define-shiftbus_load)  `PB8`<br> |
| define  | [**SHIFTBUS\_MISO**](ShiftBus_8cpp.md#define-shiftbus_miso)  `PB4`<br> |
| define  | [**SHIFTBUS\_MOSI**](ShiftBus_8cpp.md#define-shiftbus_mosi)  `PB5`<br> |
| define  | [**SHIFTBUS\_SCK**](ShiftBus_8cpp.md#define-shiftbus_sck)  `PB3`<br> |

## Detailed Description




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    
## Public Attributes Documentation




### variable ShiftBus1 

```C++
OpenSkyhawk::ShiftBus ShiftBus1(SPI, PB3, PB4, PB5, PB8, PB9);
```




<hr>
## Public Static Functions Documentation




### function kShiftBusSettings 

```C++
static const SPISettings kShiftBusSettings (
    1000000,
    MSBFIRST,
    SPI_MODE0
) 
```




<hr>
## Macro Definition Documentation





### define SHIFTBUS\_LATCH 

```C++
#define SHIFTBUS_LATCH `PB9`
```




<hr>



### define SHIFTBUS\_LOAD 

```C++
#define SHIFTBUS_LOAD `PB8`
```




<hr>



### define SHIFTBUS\_MISO 

```C++
#define SHIFTBUS_MISO `PB4`
```




<hr>



### define SHIFTBUS\_MOSI 

```C++
#define SHIFTBUS_MOSI `PB5`
```




<hr>



### define SHIFTBUS\_SCK 

```C++
#define SHIFTBUS_SCK `PB3`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Helpers/ShiftBus/ShiftBus.cpp`

