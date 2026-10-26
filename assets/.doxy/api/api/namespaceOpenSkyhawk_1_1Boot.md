

# Namespace OpenSkyhawk::Boot



[**Namespace List**](namespaces.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**Boot**](namespaceOpenSkyhawk_1_1Boot.md)






















## Public Types

| Type | Name |
| ---: | :--- |
| enum uint8\_t | [**AnnounceField**](#enum-announcefield)  <br>_Byte offsets in ANNOUNCE / CONNECTED._  |
| enum uint8\_t | [**BootState**](#enum-bootstate)  <br>_Why the board is in its bootloader (_ AnnounceField::STATE _)._ |
| enum uint8\_t | [**Cmd**](#enum-cmd)  <br> |
| enum uint8\_t | [**Reply**](#enum-reply)  <br> |
| enum uint8\_t | [**Status**](#enum-status)  <br>_Result byte of BEGUN / BLOCK\_ACK / ENDED / GOING._  |




## Public Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**ANNOUNCE\_INTERVAL\_MS**](#variable-announce_interval_ms)   = `1000`<br> |
|  constexpr uint8\_t | [**ANNOUNCE\_LEN**](#variable-announce_len)   = `8`<br> |
|  constexpr uint32\_t | [**APP\_OFFSET**](#variable-app_offset)   = `[**BOOTLOADER\_SIZE**](namespaceOpenSkyhawk_1_1Boot.md#variable-bootloader_size)`<br> |
|  constexpr uint8\_t | [**ATTEMPTS\_BKP\_REGISTER**](#variable-attempts_bkp_register)   = `2`<br> |
|  constexpr uint8\_t | [**ATTEMPT\_LIMIT**](#variable-attempt_limit)   = `3`<br> |
|  constexpr uint16\_t | [**BLOCK\_SIZE**](#variable-block_size)   = `64`<br>_bytes per block; the image is padded with 0xFF to a whole block_  |
|  constexpr uint32\_t | [**BOOTLOADER\_SIZE**](#variable-bootloader_size)   = `0x1000`<br>_4 KB = one F103 write-protect block; apps start here_  |
|  constexpr uint16\_t | [**CATCH\_WINDOW\_MS**](#variable-catch_window_ms)   = `200`<br>_after every reset, a CONNECT on BOOT\_CMD\_n keeps the board here (#364)_  |
|  constexpr uint16\_t | [**CMD\_ID\_BASE**](#variable-cmd_id_base)   = `0x740`<br>_BOOT\_CMD\_n = 0x740 + n — host (bridge) → node n's bootloader._  |
|  constexpr uint8\_t | [**DATA\_BYTES**](#variable-data_bytes)   = `7`<br>_payload bytes per DATA frame (byte 0 = Cmd::DATA\_k)_  |
|  constexpr uint8\_t | [**DATA\_FRAMES**](#variable-data_frames)   = `10`<br>_DATA frames per block (70 bytes carried, 64 used)_  |
|  constexpr uint8\_t | [**ENTRY\_BKP\_REGISTER**](#variable-entry_bkp_register)   = `1`<br> |
|  constexpr uint16\_t | [**ENTRY\_MAGIC**](#variable-entry_magic)   = `0xB007`<br> |
|  constexpr uint16\_t | [**IDLE\_TIMEOUT\_MS**](#variable-idle_timeout_ms)   = `30000`<br>_REQUESTED / CAUGHT with no command this long → back to the app (#364)_  |
|  constexpr uint8\_t | [**PROTOCOL\_VERSION**](#variable-protocol_version)   = `1`<br>_Bumped on any change to the frames below. A board reports it in READY\_n byte 3 (#365); 0 = no bootloader._  |
|  constexpr uint16\_t | [**RSP\_ID\_BASE**](#variable-rsp_id_base)   = `0x780`<br>_BOOT\_RSP\_n = 0x780 + n — node n's bootloader → host._  |
|  constexpr uint32\_t | [**UID\_ADDRESS**](#variable-uid_address)   = `0x1FFFF7E8`<br>_the F103's 96-bit unique ID_  |
|  constexpr uint16\_t | [**UNCONFIGURED\_CMD\_ID**](#variable-unconfigured_cmd_id)   = `0x7C0`<br> |
|  constexpr uint16\_t | [**UNCONFIGURED\_RSP\_ID**](#variable-unconfigured_rsp_id)   = `0x7C1`<br>_A bootloader with no NODE\_ID yet (option byte Data0 = 0xFF, no valid app) announces here._  |
















## Public Functions

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**canIdBootCmd**](#function-canidbootcmd) (uint8\_t n) <br>_BOOT\_CMD\_n for node n (0x740-0x77F)._  |
|  constexpr uint16\_t | [**canIdBootRsp**](#function-canidbootrsp) (uint8\_t n) <br>_BOOT\_RSP\_n for node n (0x780-0x7BF)._  |




























## Public Types Documentation




### enum AnnounceField 

_Byte offsets in ANNOUNCE / CONNECTED._ 
```C++
enum OpenSkyhawk::Boot::AnnounceField {
    CODE = 0,
    PROTOCOL = 1,
    ID = 2,
    KIND = 3,
    STATE = 4,
    FLASH_KB = 5,
    UID16 = 6
};
```




<hr>



### enum BootState 

_Why the board is in its bootloader (_ AnnounceField::STATE _)._
```C++
enum OpenSkyhawk::Boot::BootState {
    REQUESTED = 0x01,
    APP_INVALID = 0x02,
    UNCONFIGURED = 0x03,
    APP_UNSTABLE = 0x04,
    CAUGHT = 0x05
};
```




<hr>



### enum Cmd 

```C++
enum OpenSkyhawk::Boot::Cmd {
    CONNECT = 0x01,
    BEGIN = 0x02,
    BLOCK = 0x10,
    DATA_0 = 0x20,
    DATA_9 = 0x29,
    END = 0x30,
    GO = 0x40,
    SET_NODE_ID = 0x50
};
```




<hr>



### enum Reply 

```C++
enum OpenSkyhawk::Boot::Reply {
    CONNECTED = 0x81,
    BEGUN = 0x82,
    BLOCK_ACK = 0x90,
    ANNOUNCE = 0xA0,
    ENDED = 0xB0,
    GOING = 0xC0,
    NODE_SET = 0xD0
};
```




<hr>



### enum Status 

_Result byte of BEGUN / BLOCK\_ACK / ENDED / GOING._ 
```C++
enum OpenSkyhawk::Boot::Status {
    OK = 0x00,
    BAD_CRC = 0x01,
    FLASH_ERROR = 0x02,
    OUT_OF_RANGE = 0x03,
    OUT_OF_ORDER = 0x04,
    NO_APP = 0x05,
    BAD_IMAGE = 0x06,
    WRONG_NODE = 0x07,
    WRONG_KIND = 0x08
};
```




<hr>
## Public Attributes Documentation




### variable ANNOUNCE\_INTERVAL\_MS 

```C++
constexpr uint16_t OpenSkyhawk::Boot::ANNOUNCE_INTERVAL_MS;
```




<hr>



### variable ANNOUNCE\_LEN 

```C++
constexpr uint8_t OpenSkyhawk::Boot::ANNOUNCE_LEN;
```




<hr>



### variable APP\_OFFSET 

```C++
constexpr uint32_t OpenSkyhawk::Boot::APP_OFFSET;
```




<hr>



### variable ATTEMPTS\_BKP\_REGISTER 

```C++
constexpr uint8_t OpenSkyhawk::Boot::ATTEMPTS_BKP_REGISTER;
```



Crash-loop counter (#364): the bootloader adds one before every jump to the app; the app sets it back to 0 once it is healthy ([**STM32Board::markBootHealthy()**](namespaceSTM32Board.md#function-markboothealthy)). At ATTEMPT\_LIMIT the bootloader stays (BootState::APP\_UNSTABLE). Cleared by a committed image; lost on a power cycle. 


        

<hr>



### variable ATTEMPT\_LIMIT 

```C++
constexpr uint8_t OpenSkyhawk::Boot::ATTEMPT_LIMIT;
```




<hr>



### variable BLOCK\_SIZE 

_bytes per block; the image is padded with 0xFF to a whole block_ 
```C++
constexpr uint16_t OpenSkyhawk::Boot::BLOCK_SIZE;
```




<hr>



### variable BOOTLOADER\_SIZE 

_4 KB = one F103 write-protect block; apps start here_ 
```C++
constexpr uint32_t OpenSkyhawk::Boot::BOOTLOADER_SIZE;
```




<hr>



### variable CATCH\_WINDOW\_MS 

_after every reset, a CONNECT on BOOT\_CMD\_n keeps the board here (#364)_ 
```C++
constexpr uint16_t OpenSkyhawk::Boot::CATCH_WINDOW_MS;
```




<hr>



### variable CMD\_ID\_BASE 

_BOOT\_CMD\_n = 0x740 + n — host (bridge) → node n's bootloader._ 
```C++
constexpr uint16_t OpenSkyhawk::Boot::CMD_ID_BASE;
```




<hr>



### variable DATA\_BYTES 

_payload bytes per DATA frame (byte 0 = Cmd::DATA\_k)_ 
```C++
constexpr uint8_t OpenSkyhawk::Boot::DATA_BYTES;
```




<hr>



### variable DATA\_FRAMES 

_DATA frames per block (70 bytes carried, 64 used)_ 
```C++
constexpr uint8_t OpenSkyhawk::Boot::DATA_FRAMES;
```




<hr>



### variable ENTRY\_BKP\_REGISTER 

```C++
constexpr uint8_t OpenSkyhawk::Boot::ENTRY_BKP_REGISTER;
```



The app sets backup register DR1 to ENTRY\_MAGIC and resets to enter the bootloader (#365). The bootloader clears it on read. The backup domain survives a reset, not a power cycle. 


        

<hr>



### variable ENTRY\_MAGIC 

```C++
constexpr uint16_t OpenSkyhawk::Boot::ENTRY_MAGIC;
```




<hr>



### variable IDLE\_TIMEOUT\_MS 

_REQUESTED / CAUGHT with no command this long → back to the app (#364)_ 
```C++
constexpr uint16_t OpenSkyhawk::Boot::IDLE_TIMEOUT_MS;
```




<hr>



### variable PROTOCOL\_VERSION 

_Bumped on any change to the frames below. A board reports it in READY\_n byte 3 (#365); 0 = no bootloader._ 
```C++
constexpr uint8_t OpenSkyhawk::Boot::PROTOCOL_VERSION;
```




<hr>



### variable RSP\_ID\_BASE 

_BOOT\_RSP\_n = 0x780 + n — node n's bootloader → host._ 
```C++
constexpr uint16_t OpenSkyhawk::Boot::RSP_ID_BASE;
```




<hr>



### variable UID\_ADDRESS 

_the F103's 96-bit unique ID_ 
```C++
constexpr uint32_t OpenSkyhawk::Boot::UID_ADDRESS;
```




<hr>



### variable UNCONFIGURED\_CMD\_ID 

```C++
constexpr uint16_t OpenSkyhawk::Boot::UNCONFIGURED_CMD_ID;
```



Commands to an unconfigured bootloader (#364). CONNECT names the board's UID hash (AnnounceField::UID16); only that board binds the session and answers on UNCONFIGURED\_RSP\_ID. One unconfigured board on the bus at a time. 


        

<hr>



### variable UNCONFIGURED\_RSP\_ID 

_A bootloader with no NODE\_ID yet (option byte Data0 = 0xFF, no valid app) announces here._ 
```C++
constexpr uint16_t OpenSkyhawk::Boot::UNCONFIGURED_RSP_ID;
```




<hr>
## Public Functions Documentation




### function canIdBootCmd 

_BOOT\_CMD\_n for node n (0x740-0x77F)._ 
```C++
constexpr uint16_t OpenSkyhawk::Boot::canIdBootCmd (
    uint8_t n
) 
```




<hr>



### function canIdBootRsp 

_BOOT\_RSP\_n for node n (0x780-0x7BF)._ 
```C++
constexpr uint16_t OpenSkyhawk::Boot::canIdBootRsp (
    uint8_t n
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/BootProtocol/BootProtocol.h`

