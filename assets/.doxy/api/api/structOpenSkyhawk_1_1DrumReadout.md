

# Struct OpenSkyhawk::DrumReadout



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**DrumReadout**](structOpenSkyhawk_1_1DrumReadout.md)



_Complete description of one rolling readout: its sources, geometry, glyphs, flag._ [More...](#detailed-description)

* `#include <DrumDisplay.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  float | [**digitHeightMm**](#variable-digitheightmm)  <br>_digit cell (roll window) height, mm_  |
|  float | [**digitWidthMm**](#variable-digitwidthmm)  <br>_digit cell (window aperture) width, mm_  |
|  [**DrumFlag**](structOpenSkyhawk_1_1DrumFlag.md) | [**flag**](#variable-flag)  <br>_optional flag (flag.enabled == false ⇒ none)_  |
|  const [**DrumGlyph**](structOpenSkyhawk_1_1DrumGlyph.md) \* | [**glyphs**](#variable-glyphs)  <br>_fixed glyphs (decimal point etc.); nullptr if none_  |
|  float | [**groupGapMm**](#variable-groupgapmm)  <br>_extra gap at group boundaries, mm (0 if ungrouped)_  |
|  uint8\_t | [**groupSize**](#variable-groupsize)  <br>_digits per group for groupGap insertion (0 = no grouping)_  |
|  float | [**interDigitGapMm**](#variable-interdigitgapmm)  <br>_gap between adjacent cells, mm_  |
|  uint8\_t | [**nDigits**](#variable-ndigits)  <br>_total digit columns in the combined number (1..6)_  |
|  uint8\_t | [**nGlyphs**](#variable-nglyphs)  <br>_element count of_ `glyphs` __ |
|  uint8\_t | [**nSources**](#variable-nsources)  <br>_element count of_ `sources` __ |
|  [**DrumScroll**](namespaceOpenSkyhawk.md#enum-drumscroll) | [**scroll**](#variable-scroll)  <br>_EASE\_ONLY or SNAP\_SETTLE._  |
|  float | [**snapThreshold**](#variable-snapthreshold)  <br>_\|target−pos\| (digit units) above which SNAP\_SETTLE teleports_  |
|  const [**DrumSource**](structOpenSkyhawk_1_1DrumSource.md) \* | [**sources**](#variable-sources)  <br>_array of digit sources_  |












































## Detailed Description




**Note:**

All spatial fields are mm; [**DrumDisplay**](classOpenSkyhawk_1_1DrumDisplay.md) converts to px at configure() from the panel's getDisplayWidth()/getDisplayHeight(). `digitWidthMm` / `digitHeightMm` are the drum-window aperture. If the laid-out row is wider than the panel, auto-fit shrinks the pitch to the pixel width. 





    
## Public Attributes Documentation




### variable digitHeightMm 

_digit cell (roll window) height, mm_ 
```C++
float OpenSkyhawk::DrumReadout::digitHeightMm;
```




<hr>



### variable digitWidthMm 

_digit cell (window aperture) width, mm_ 
```C++
float OpenSkyhawk::DrumReadout::digitWidthMm;
```




<hr>



### variable flag 

_optional flag (flag.enabled == false ⇒ none)_ 
```C++
DrumFlag OpenSkyhawk::DrumReadout::flag;
```




<hr>



### variable glyphs 

_fixed glyphs (decimal point etc.); nullptr if none_ 
```C++
const DrumGlyph* OpenSkyhawk::DrumReadout::glyphs;
```




<hr>



### variable groupGapMm 

_extra gap at group boundaries, mm (0 if ungrouped)_ 
```C++
float OpenSkyhawk::DrumReadout::groupGapMm;
```




<hr>



### variable groupSize 

_digits per group for groupGap insertion (0 = no grouping)_ 
```C++
uint8_t OpenSkyhawk::DrumReadout::groupSize;
```




<hr>



### variable interDigitGapMm 

_gap between adjacent cells, mm_ 
```C++
float OpenSkyhawk::DrumReadout::interDigitGapMm;
```




<hr>



### variable nDigits 

_total digit columns in the combined number (1..6)_ 
```C++
uint8_t OpenSkyhawk::DrumReadout::nDigits;
```




<hr>



### variable nGlyphs 

_element count of_ `glyphs` __
```C++
uint8_t OpenSkyhawk::DrumReadout::nGlyphs;
```




<hr>



### variable nSources 

_element count of_ `sources` __
```C++
uint8_t OpenSkyhawk::DrumReadout::nSources;
```




<hr>



### variable scroll 

_EASE\_ONLY or SNAP\_SETTLE._ 
```C++
DrumScroll OpenSkyhawk::DrumReadout::scroll;
```




<hr>



### variable snapThreshold 

_\|target−pos\| (digit units) above which SNAP\_SETTLE teleports_ 
```C++
float OpenSkyhawk::DrumReadout::snapThreshold;
```




<hr>



### variable sources 

_array of digit sources_ 
```C++
const DrumSource* OpenSkyhawk::DrumReadout::sources;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/DrumDisplay/DrumDisplay.h`

