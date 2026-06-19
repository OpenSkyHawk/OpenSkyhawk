

# Struct OpenSkyhawk::DrumGlyph



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**DrumGlyph**](structOpenSkyhawk_1_1DrumGlyph.md)



_A fixed (non-rolling) glyph painted between digit columns — '.', ' ', ':' etc._ [More...](#detailed-description)

* `#include <DrumDisplay.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint8\_t | [**afterCol**](#variable-aftercol)  <br>_visual column it follows: inserted after this many digits from the left_  |
|  char | [**ch**](#variable-ch)  <br>_literal glyph to paint_  |
|  float | [**widthMm**](#variable-widthmm)  <br>_cell width for this glyph, mm (narrower than a digit, e.g. 1.8 for '.')_  |












































## Detailed Description




**Note:**

Drawn statically (no tape) in its own cell. Used for the ARC-51 / altimeter decimal. 





    
## Public Attributes Documentation




### variable afterCol 

_visual column it follows: inserted after this many digits from the left_ 
```C++
uint8_t OpenSkyhawk::DrumGlyph::afterCol;
```




<hr>



### variable ch 

_literal glyph to paint_ 
```C++
char OpenSkyhawk::DrumGlyph::ch;
```




<hr>



### variable widthMm 

_cell width for this glyph, mm (narrower than a digit, e.g. 1.8 for '.')_ 
```C++
float OpenSkyhawk::DrumGlyph::widthMm;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/DrumDisplay/DrumDisplay.h`

