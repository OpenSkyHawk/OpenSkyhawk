

# Struct OpenSkyhawk::FirmwareImage::Identity



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**FirmwareImage**](namespaceOpenSkyhawk_1_1FirmwareImage.md) **>** [**Identity**](structOpenSkyhawk_1_1FirmwareImage_1_1Identity.md)





* `#include <FirmwareImage.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint16\_t | [**blockSize**](#variable-blocksize)  <br>_6 sizeof(Identity); a newer layout may grow — readers parse the prefix they know_  |
|  uint8\_t | [**fwMajor**](#variable-fwmajor)  <br>_18_ [_**FirmwareVersion.h**_](FirmwareVersion_8h.md) _— the same version READY\_n reports_ |
|  uint8\_t | [**fwMinor**](#variable-fwminor)  <br>_19_  |
|  uint8\_t | [**fwPatch**](#variable-fwpatch)  <br>_20_  |
|  uint32\_t | [**imageCrc**](#variable-imagecrc)  <br>_12 CRC-32/MPEG-2 over imageLength bytes, this word skipped — stamped after linking_  |
|  uint32\_t | [**imageLength**](#variable-imagelength)  <br>_8 bytes from the app's start, padded to 4 — stamped after linking_  |
|  uint16\_t | [**layoutVersion**](#variable-layoutversion)  <br>_4 LAYOUT\_VERSION_  |
|  uint32\_t | [**magic**](#variable-magic)  <br>_0 MAGIC_  |
|  uint8\_t | [**nodeId**](#variable-nodeid)  <br>_16 NODE\_ID (the build flag, D10)_  |
|  uint8\_t | [**nodeKind**](#variable-nodekind)  <br>_17 NodeKind of the role_  |
|  uint8\_t | [**reserved**](#variable-reserved)  <br>_21 0_  |












































## Public Attributes Documentation




### variable blockSize 

_6 sizeof(Identity); a newer layout may grow — readers parse the prefix they know_ 
```C++
uint16_t OpenSkyhawk::FirmwareImage::Identity::blockSize;
```




<hr>



### variable fwMajor 

_18_ [_**FirmwareVersion.h**_](FirmwareVersion_8h.md) _— the same version READY\_n reports_
```C++
uint8_t OpenSkyhawk::FirmwareImage::Identity::fwMajor;
```




<hr>



### variable fwMinor 

_19_ 
```C++
uint8_t OpenSkyhawk::FirmwareImage::Identity::fwMinor;
```




<hr>



### variable fwPatch 

_20_ 
```C++
uint8_t OpenSkyhawk::FirmwareImage::Identity::fwPatch;
```




<hr>



### variable imageCrc 

_12 CRC-32/MPEG-2 over imageLength bytes, this word skipped — stamped after linking_ 
```C++
uint32_t OpenSkyhawk::FirmwareImage::Identity::imageCrc;
```




<hr>



### variable imageLength 

_8 bytes from the app's start, padded to 4 — stamped after linking_ 
```C++
uint32_t OpenSkyhawk::FirmwareImage::Identity::imageLength;
```




<hr>



### variable layoutVersion 

_4 LAYOUT\_VERSION_ 
```C++
uint16_t OpenSkyhawk::FirmwareImage::Identity::layoutVersion;
```




<hr>



### variable magic 

_0 MAGIC_ 
```C++
uint32_t OpenSkyhawk::FirmwareImage::Identity::magic;
```




<hr>



### variable nodeId 

_16 NODE\_ID (the build flag, D10)_ 
```C++
uint8_t OpenSkyhawk::FirmwareImage::Identity::nodeId;
```




<hr>



### variable nodeKind 

_17 NodeKind of the role_ 
```C++
uint8_t OpenSkyhawk::FirmwareImage::Identity::nodeKind;
```




<hr>



### variable reserved 

_21 0_ 
```C++
uint8_t OpenSkyhawk::FirmwareImage::Identity::reserved[11];
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/FirmwareImage/FirmwareImage.h`

