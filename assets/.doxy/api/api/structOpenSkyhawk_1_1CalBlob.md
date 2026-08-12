

# Struct OpenSkyhawk::CalBlob



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**CalBlob**](structOpenSkyhawk_1_1CalBlob.md)



_The whole persisted calibration set, written and erased as one unit._ [More...](#detailed-description)

* `#include <AxisCal.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  [**AxisCal**](structOpenSkyhawk_1_1AxisCal.md) | [**axes**](#variable-axes)  <br>_offset 6 — 64 bytes_  |
|  uint16\_t | [**crc**](#variable-crc)  <br>_offset 70 — CRC-16/CCITT-FALSE over bytes [0, 70)_  |
|  uint32\_t | [**magic**](#variable-magic)  <br>_offset 0 — CAL\_MAGIC_  |
|  uint16\_t | [**version**](#variable-version)  <br>_offset 4 — CAL\_VERSION_  |












































## Detailed Description


Offsets are natural-alignment under ARM EABI and happen to pack tight with no padding. That is luck rather than design, which is why the static\_asserts below are load-bearing — `crc` is computed over `offsetof(CalBlob, crc)` bytes, so a layout change silently invalidates every stored blob unless CAL\_VERSION moves with it.




**Note:**

Not `__attribute__((packed))`. There is no padding to remove, and packing would make every field access unaligned on a Cortex-M0+, which has no unaligned load/store. 





    
## Public Attributes Documentation




### variable axes 

_offset 6 — 64 bytes_ 
```C++
AxisCal OpenSkyhawk::CalBlob::axes[AXIS_CAL_SLOTS];
```




<hr>



### variable crc 

_offset 70 — CRC-16/CCITT-FALSE over bytes [0, 70)_ 
```C++
uint16_t OpenSkyhawk::CalBlob::crc;
```




<hr>



### variable magic 

_offset 0 — CAL\_MAGIC_ 
```C++
uint32_t OpenSkyhawk::CalBlob::magic;
```




<hr>



### variable version 

_offset 4 — CAL\_VERSION_ 
```C++
uint16_t OpenSkyhawk::CalBlob::version;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/AxisCal.h`

