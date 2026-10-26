

# Struct OpenSkyhawk::Boot::BootloaderInfo



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**Boot**](namespaceOpenSkyhawk_1_1Boot.md) **>** [**BootloaderInfo**](structOpenSkyhawk_1_1Boot_1_1BootloaderInfo.md)





* `#include <BootProtocol.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint32\_t | [**magic**](#variable-magic)  <br>_INFO\_MAGIC._  |
|  uint8\_t | [**protocolVersion**](#variable-protocolversion)  <br>_PROTOCOL\_VERSION of the bootloader that is installed._  |
|  uint8\_t | [**reserved**](#variable-reserved)  <br>_0xFF_  |












































## Public Attributes Documentation




### variable magic 

_INFO\_MAGIC._ 
```C++
uint32_t OpenSkyhawk::Boot::BootloaderInfo::magic;
```




<hr>



### variable protocolVersion 

_PROTOCOL\_VERSION of the bootloader that is installed._ 
```C++
uint8_t OpenSkyhawk::Boot::BootloaderInfo::protocolVersion;
```




<hr>



### variable reserved 

_0xFF_ 
```C++
uint8_t OpenSkyhawk::Boot::BootloaderInfo::reserved[11];
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/BootProtocol/BootProtocol.h`

