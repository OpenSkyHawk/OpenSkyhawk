

# Struct OpenSkyhawk::DrumFlag



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**DrumFlag**](structOpenSkyhawk_1_1DrumFlag.md)



_Optional 2-state (or N-state) flag tape — hemisphere N/S · E/W, or a mode letter._ [More...](#detailed-description)

* `#include <DrumDisplay.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint16\_t | [**address**](#variable-address)  <br>_DCS-BIOS address that drives the flag._  |
|  uint8\_t | [**atVisualCol**](#variable-atvisualcol)  <br>_visual column the flag cell is inserted at (after this many digits)_  |
|  bool | [**enabled**](#variable-enabled)  <br>_false ⇒ no flag tape rendered (default)_  |
|  const char \* | [**faces**](#variable-faces)  <br>_face string, one char per state, e.g. "NS" / "EW" (nFaces = strlen)_  |
|  uint16\_t | [**mask**](#variable-mask)  <br>_field mask (A\_4E\_C\_\*\_AM); 0xFFFF for whole-word_  |
|  uint16\_t | [**steps**](#variable-steps)   = `0`<br>_positions across the range; 0 ⇒ nFaces−1 (a full-scale flag)_  |
|  float | [**widthMm**](#variable-widthmm)  <br>_flag cell width, mm (wider than a digit so a broad 'W' fits)_  |












































## Detailed Description




**Note:**

OFF by default (`enabled` = false). Position is configurable (not hardcoded rightmost). Populate `address` / `mask` from the A4EC constants. The face is round(value/mask · steps), clamped to the face count. 




**Note:**

`steps` matters because the A-4E-C does not drive every flag over the same range: the ASN-41 hemisphere arrives at HALF scale (nav.lua sets 0.0 or 0.5 for N/S and E/W, so steps = 2), while MagVar and the BDHI DME flag use the full range (0 or 1, so steps = 1). A half-scale flag read as full scale never reaches its second face. 





    
## Public Attributes Documentation




### variable address 

_DCS-BIOS address that drives the flag._ 
```C++
uint16_t OpenSkyhawk::DrumFlag::address;
```




<hr>



### variable atVisualCol 

_visual column the flag cell is inserted at (after this many digits)_ 
```C++
uint8_t OpenSkyhawk::DrumFlag::atVisualCol;
```




<hr>



### variable enabled 

_false ⇒ no flag tape rendered (default)_ 
```C++
bool OpenSkyhawk::DrumFlag::enabled;
```




<hr>



### variable faces 

_face string, one char per state, e.g. "NS" / "EW" (nFaces = strlen)_ 
```C++
const char* OpenSkyhawk::DrumFlag::faces;
```




<hr>



### variable mask 

_field mask (A\_4E\_C\_\*\_AM); 0xFFFF for whole-word_ 
```C++
uint16_t OpenSkyhawk::DrumFlag::mask;
```




<hr>



### variable steps 

_positions across the range; 0 ⇒ nFaces−1 (a full-scale flag)_ 
```C++
uint16_t OpenSkyhawk::DrumFlag::steps;
```




<hr>



### variable widthMm 

_flag cell width, mm (wider than a digit so a broad 'W' fits)_ 
```C++
float OpenSkyhawk::DrumFlag::widthMm;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/DrumDisplay/DrumDisplay.h`

