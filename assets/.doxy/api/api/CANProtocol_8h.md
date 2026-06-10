

# File CANProtocol.h



[**FileList**](files.md) **>** [**CANProtocol**](dir_81ff3032570f78b12938068450b63228.md) **>** [**CANProtocol.h**](CANProtocol_8h.md)

[Go to the source code of this file](CANProtocol_8h_source.md)



* `#include <stdint.h>`
* `#include <HIDControls.h>`

















## Public Types

| Type | Name |
| ---: | :--- |
| enum  | [**CanStatus**](#enum-canstatus)  <br>_CAN bus health states reported to_ [_**STM32Board**_](namespaceSTM32Board.md) _via onCanStatus()._ |






## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint32\_t | [**CAN\_ID\_CTRL\_BCAST**](#variable-can_id_ctrl_bcast)   = `0x010`<br> |
|  constexpr uint32\_t | [**CAN\_ID\_ECHO\_1**](#variable-can_id_echo_1)   = `0x210`<br> |
|  constexpr uint32\_t | [**CAN\_ID\_ECHO\_2**](#variable-can_id_echo_2)   = `0x211`<br> |
|  constexpr uint32\_t | [**CAN\_ID\_EVT\_1**](#variable-can_id_evt_1)   = `0x200`<br> |
|  constexpr uint32\_t | [**CAN\_ID\_EVT\_2**](#variable-can_id_evt_2)   = `0x201`<br> |
|  constexpr uint32\_t | [**CAN\_ID\_HB\_1**](#variable-can_id_hb_1)   = `0x100`<br> |
|  constexpr uint32\_t | [**CAN\_ID\_HB\_2**](#variable-can_id_hb_2)   = `0x101`<br> |
|  constexpr uint32\_t | [**CAN\_ID\_TEST\_SEQ**](#variable-can_id_test_seq)   = `0x011`<br> |
|  constexpr uint16\_t | [**CTRL\_TEST\_SEQ**](#variable-ctrl_test_seq)   = `0xFFFF`<br> |
|  constexpr uint8\_t | [**DIAG\_ERR**](#variable-diag_err)   = `0x03`<br> |
|  constexpr uint8\_t | [**DIAG\_EVT**](#variable-diag_evt)   = `0x04`<br> |
|  constexpr uint8\_t | [**DIAG\_HB**](#variable-diag_hb)   = `0x02`<br> |
|  constexpr uint8\_t | [**DIAG\_MAGIC**](#variable-diag_magic)   = `0xAA`<br> |
|  constexpr uint8\_t | [**DIAG\_RTT**](#variable-diag_rtt)   = `0x01`<br> |














## Public Functions

| Type | Name |
| ---: | :--- |
|  struct | [**\_\_attribute\_\_**](#function-__attribute__) ((packed)) <br> |




























## Public Types Documentation




### enum CanStatus 

_CAN bus health states reported to_ [_**STM32Board**_](namespaceSTM32Board.md) _via onCanStatus()._
```C++
enum CanStatus {
    STARTING,
    NORMAL,
    TX_ERROR,
    BUS_OFF
};
```




<hr>
## Public Static Attributes Documentation




### variable CAN\_ID\_CTRL\_BCAST 

```C++
constexpr uint32_t CAN_ID_CTRL_BCAST;
```




<hr>



### variable CAN\_ID\_ECHO\_1 

```C++
constexpr uint32_t CAN_ID_ECHO_1;
```




<hr>



### variable CAN\_ID\_ECHO\_2 

```C++
constexpr uint32_t CAN_ID_ECHO_2;
```




<hr>



### variable CAN\_ID\_EVT\_1 

```C++
constexpr uint32_t CAN_ID_EVT_1;
```




<hr>



### variable CAN\_ID\_EVT\_2 

```C++
constexpr uint32_t CAN_ID_EVT_2;
```




<hr>



### variable CAN\_ID\_HB\_1 

```C++
constexpr uint32_t CAN_ID_HB_1;
```




<hr>



### variable CAN\_ID\_HB\_2 

```C++
constexpr uint32_t CAN_ID_HB_2;
```




<hr>



### variable CAN\_ID\_TEST\_SEQ 

```C++
constexpr uint32_t CAN_ID_TEST_SEQ;
```




<hr>



### variable CTRL\_TEST\_SEQ 

```C++
constexpr uint16_t CTRL_TEST_SEQ;
```




<hr>



### variable DIAG\_ERR 

```C++
constexpr uint8_t DIAG_ERR;
```




<hr>



### variable DIAG\_EVT 

```C++
constexpr uint8_t DIAG_EVT;
```




<hr>



### variable DIAG\_HB 

```C++
constexpr uint8_t DIAG_HB;
```




<hr>



### variable DIAG\_MAGIC 

```C++
constexpr uint8_t DIAG_MAGIC;
```




<hr>



### variable DIAG\_RTT 

```C++
constexpr uint8_t DIAG_RTT;
```




<hr>
## Public Functions Documentation




### function \_\_attribute\_\_ 

```C++
struct __attribute__ (
    (packed)
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/CANProtocol/CANProtocol.h`

