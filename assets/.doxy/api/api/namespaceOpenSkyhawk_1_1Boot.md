

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
|  constexpr uint8\_t | [**ANNOUNCE\_LEN**](#variable-announce_len)   = `6`<br> |
|  constexpr uint32\_t | [**APP\_OFFSET**](#variable-app_offset)   = `[**BOOTLOADER\_SIZE**](namespaceOpenSkyhawk_1_1Boot.md#variable-bootloader_size)`<br> |
|  constexpr uint16\_t | [**BLOCK\_SIZE**](#variable-block_size)   = `64`<br>_bytes per block; the image is padded with 0xFF to a whole block_  |
|  constexpr uint32\_t | [**BOOTLOADER\_SIZE**](#variable-bootloader_size)   = `0x1000`<br>_4 KB = one F103 write-protect block; apps start here_  |
|  constexpr uint16\_t | [**CMD\_ID\_BASE**](#variable-cmd_id_base)   = `0x740`<br>_BOOT\_CMD\_n = 0x740 + n — host (bridge) → node n's bootloader._  |
|  constexpr uint8\_t | [**DATA\_BYTES**](#variable-data_bytes)   = `7`<br>_payload bytes per DATA frame (byte 0 = Cmd::DATA\_k)_  |
|  constexpr uint8\_t | [**DATA\_FRAMES**](#variable-data_frames)   = `10`<br>_DATA frames per block (70 bytes carried, 64 used)_  |
|  constexpr uint8\_t | [**ENTRY\_BKP\_REGISTER**](#variable-entry_bkp_register)   = `1`<br> |
|  constexpr uint16\_t | [**ENTRY\_MAGIC**](#variable-entry_magic)   = `0xB007`<br> |
|  constexpr uint8\_t | [**PROTOCOL\_VERSION**](#variable-protocol_version)   = `1`<br>_Bumped on any change to the frames below. A board reports it in READY\_n byte 3 (#365); 0 = no bootloader._  |
|  constexpr uint16\_t | [**RSP\_ID\_BASE**](#variable-rsp_id_base)   = `0x780`<br>_BOOT\_RSP\_n = 0x780 + n — node n's bootloader → host._  |
|  constexpr uint16\_t | [**UNCONFIGURED\_CMD\_ID**](#variable-unconfigured_cmd_id)   = `0x7C0`<br>_Commands to an unconfigured bootloader, addressed by chip UID — reserved for #364, unused in #363._  |
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
    NODE_ID = 2,
    KIND = 3,
    STATE = 4,
    FLASH_KB = 5
};
```




<hr>



### enum BootState 

_Why the board is in its bootloader (_ AnnounceField::STATE _)._
```C++
enum OpenSkyhawk::Boot::BootState {
    REQUESTED = 0x01,
    APP_INVALID = 0x02,
    UNCONFIGURED = 0x03
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
    GO = 0x40
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
    GOING = 0xC0
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
    BAD_IMAGE = 0x06
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



### variable UNCONFIGURED\_CMD\_ID 

_Commands to an unconfigured bootloader, addressed by chip UID — reserved for #364, unused in #363._ 
```C++
constexpr uint16_t OpenSkyhawk::Boot::UNCONFIGURED_CMD_ID;
```




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

