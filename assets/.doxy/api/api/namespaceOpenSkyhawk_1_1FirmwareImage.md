

# Namespace OpenSkyhawk::FirmwareImage



[**Namespace List**](namespaces.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**FirmwareImage**](namespaceOpenSkyhawk_1_1FirmwareImage.md)




















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**Identity**](structOpenSkyhawk_1_1FirmwareImage_1_1Identity.md) <br> |






## Public Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint32\_t | [**CRC\_VECTOR\_BYTES\_00\_0F**](#variable-crc_vector_bytes_00_0f)   = `0x081B46CAu`<br>_bytes 0x00..0x0F (four words)_  |
|  constexpr uint32\_t | [**CRC\_VECTOR\_WORD\_12345678**](#variable-crc_vector_word_12345678)   = `0xDF8A8A2Bu`<br>_one word, 0x12345678_  |
|  constexpr uint32\_t | [**IDENTITY\_OFFSET**](#variable-identity_offset)   = `0x200`<br>_from the app's start (the vector table's address)_  |
|  constexpr uint16\_t | [**LAYOUT\_VERSION**](#variable-layout_version)   = `1`<br> |
|  constexpr uint32\_t | [**MAGIC**](#variable-magic)   = `0x594B534Fu`<br>_"OSKY" in memory order_  |
|  constexpr uint32\_t | [**OFF\_CRC**](#variable-off_crc)   = `12`<br> |
|  constexpr uint32\_t | [**OFF\_FW\_MAJOR**](#variable-off_fw_major)   = `18`<br> |
|  constexpr uint32\_t | [**OFF\_FW\_MINOR**](#variable-off_fw_minor)   = `19`<br> |
|  constexpr uint32\_t | [**OFF\_FW\_PATCH**](#variable-off_fw_patch)   = `20`<br> |
|  constexpr uint32\_t | [**OFF\_LAYOUT**](#variable-off_layout)   = `4`<br> |
|  constexpr uint32\_t | [**OFF\_LENGTH**](#variable-off_length)   = `8`<br> |
|  constexpr uint32\_t | [**OFF\_MAGIC**](#variable-off_magic)   = `0`<br> |
|  constexpr uint32\_t | [**OFF\_NODE\_ID**](#variable-off_node_id)   = `16`<br> |
|  constexpr uint32\_t | [**OFF\_NODE\_KIND**](#variable-off_node_kind)   = `17`<br> |
|  constexpr uint32\_t | [**OFF\_SIZE**](#variable-off_size)   = `6`<br> |
|  constexpr uint32\_t | [**UNSTAMPED**](#variable-unstamped)   = `0xFFFFFFFFu`<br>_imageLength / imageCrc before osh\_image.py runs_  |












































## Public Attributes Documentation




### variable CRC\_VECTOR\_BYTES\_00\_0F 

_bytes 0x00..0x0F (four words)_ 
```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::CRC_VECTOR_BYTES_00_0F;
```




<hr>



### variable CRC\_VECTOR\_WORD\_12345678 

_one word, 0x12345678_ 
```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::CRC_VECTOR_WORD_12345678;
```




<hr>



### variable IDENTITY\_OFFSET 

_from the app's start (the vector table's address)_ 
```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::IDENTITY_OFFSET;
```




<hr>



### variable LAYOUT\_VERSION 

```C++
constexpr uint16_t OpenSkyhawk::FirmwareImage::LAYOUT_VERSION;
```




<hr>



### variable MAGIC 

_"OSKY" in memory order_ 
```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::MAGIC;
```




<hr>



### variable OFF\_CRC 

```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::OFF_CRC;
```




<hr>



### variable OFF\_FW\_MAJOR 

```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::OFF_FW_MAJOR;
```




<hr>



### variable OFF\_FW\_MINOR 

```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::OFF_FW_MINOR;
```




<hr>



### variable OFF\_FW\_PATCH 

```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::OFF_FW_PATCH;
```




<hr>



### variable OFF\_LAYOUT 

```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::OFF_LAYOUT;
```




<hr>



### variable OFF\_LENGTH 

```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::OFF_LENGTH;
```




<hr>



### variable OFF\_MAGIC 

```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::OFF_MAGIC;
```




<hr>



### variable OFF\_NODE\_ID 

```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::OFF_NODE_ID;
```




<hr>



### variable OFF\_NODE\_KIND 

```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::OFF_NODE_KIND;
```




<hr>



### variable OFF\_SIZE 

```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::OFF_SIZE;
```




<hr>



### variable UNSTAMPED 

_imageLength / imageCrc before osh\_image.py runs_ 
```C++
constexpr uint32_t OpenSkyhawk::FirmwareImage::UNSTAMPED;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/FirmwareImage/FirmwareImage.h`

