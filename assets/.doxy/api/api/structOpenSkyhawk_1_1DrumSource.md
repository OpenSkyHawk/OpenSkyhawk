

# Struct OpenSkyhawk::DrumSource



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**DrumSource**](structOpenSkyhawk_1_1DrumSource.md)



_One DCS-BIOS digit source feeding a_ [_**DrumReadout**_](structOpenSkyhawk_1_1DrumReadout.md) _._[More...](#detailed-description)

* `#include <DrumDisplay.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint16\_t | [**address**](#variable-address)  <br>_DCS-BIOS output address (A\_4E\_C\_\* from_ [_**A4EC\_OutputIds.h**_](A4EC__OutputIds_8h.md) _)_ |
|  uint16\_t | [**mask**](#variable-mask)  <br>_field mask (A\_4E\_C\_\*\_AM); 0xFFFF for whole-word sources_  |
|  uint8\_t | [**nDigits**](#variable-ndigits)  <br>_digits this address encodes (1 for APN153, 2 for ARC-51 groups)_  |
|  uint8\_t | [**place**](#variable-place)  <br>_least-significant digit column this source writes (0 = rightmost)_  |












































## Detailed Description




**Note:**

Populate `address` / `mask` from the A\_4E\_C\_\* and A\_4E\_C\_\*\_AM constants in [**A4EC\_OutputIds.h**](A4EC__OutputIds_8h.md). The masked value (0..mask) is scaled to nDigits decimal digits: digits = round((value & mask) / mask \* (10^nDigits − 1)); each of those nDigits digits is spliced into the readout's combined number starting at `place` (0 = least-significant column). 





    
## Public Attributes Documentation




### variable address 

_DCS-BIOS output address (A\_4E\_C\_\* from_ [_**A4EC\_OutputIds.h**_](A4EC__OutputIds_8h.md) _)_
```C++
uint16_t OpenSkyhawk::DrumSource::address;
```




<hr>



### variable mask 

_field mask (A\_4E\_C\_\*\_AM); 0xFFFF for whole-word sources_ 
```C++
uint16_t OpenSkyhawk::DrumSource::mask;
```




<hr>



### variable nDigits 

_digits this address encodes (1 for APN153, 2 for ARC-51 groups)_ 
```C++
uint8_t OpenSkyhawk::DrumSource::nDigits;
```




<hr>



### variable place 

_least-significant digit column this source writes (0 = rightmost)_ 
```C++
uint8_t OpenSkyhawk::DrumSource::place;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/DrumDisplay/DrumDisplay.h`

