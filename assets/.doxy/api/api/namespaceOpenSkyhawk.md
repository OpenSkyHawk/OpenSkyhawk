

# Namespace OpenSkyhawk



[**Namespace List**](namespaces.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md)




















## Classes

| Type | Name |
| ---: | :--- |
| class | [**DrumDisplay**](classOpenSkyhawk_1_1DrumDisplay.md) <br>_Rolling-drum OLED readout. One instance == one OLED panel._  |
| struct | [**DrumFlag**](structOpenSkyhawk_1_1DrumFlag.md) <br>_Optional 2-state (or N-state) flag tape — hemisphere N/S · E/W, or a mode letter._  |
| struct | [**DrumGlyph**](structOpenSkyhawk_1_1DrumGlyph.md) <br>_A fixed (non-rolling) glyph painted between digit columns — '.', ' ', ':' etc._  |
| struct | [**DrumReadout**](structOpenSkyhawk_1_1DrumReadout.md) <br>_Complete description of one rolling readout: its sources, geometry, glyphs, flag._  |
| struct | [**DrumSource**](structOpenSkyhawk_1_1DrumSource.md) <br>_One DCS-BIOS digit source feeding a_ [_**DrumReadout**_](structOpenSkyhawk_1_1DrumReadout.md) _._ |
| class | [**HIDAxis**](classOpenSkyhawk_1_1HIDAxis.md) <br>_HID axis handler. Declared at sketch scope for each joystick axis._  |
| class | [**HIDButton**](classOpenSkyhawk_1_1HIDButton.md) <br>_HID button handler. Declared at sketch scope for each button._  |
| class | [**HIDHatSwitch**](classOpenSkyhawk_1_1HIDHatSwitch.md) <br>_HID hat switch handler. Declared at sketch scope for each hat switch._  |
| class | [**I2cMux**](classOpenSkyhawk_1_1I2cMux.md) <br>_Selects one downstream channel of a TCA9548A I2C multiplexer._  |
| class | [**InputBase**](classOpenSkyhawk_1_1InputBase.md) <br>_Abstract base for all hardware-polled input objects._  |
| class | [**LED**](classOpenSkyhawk_1_1LED.md) <br>_Digital_ [_**LED**_](classOpenSkyhawk_1_1LED.md) _output. Drives a pin based on a DCS-BIOS state value._ |
| class | [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) <br>_Abstract base for all DCS-driven output objects._  |
| class | [**Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md) <br>_Debounced 2-position switch. Self-registers into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._ |


## Public Types

| Type | Name |
| ---: | :--- |
| enum uint8\_t | [**DrumFont**](#enum-drumfont)  <br>_Glyph font size. Maps to a fixed monospace ProFont face._  |
| enum uint8\_t | [**DrumScroll**](#enum-drumscroll)  <br>_Scroll behaviour per readout._  |






## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  const float | [**EASE**](#variable-ease)   = `0.30f`<br> |
|  const uint32\_t | [**FRAME\_MS**](#variable-frame_ms)   = `16`<br> |
|  const uint8\_t | [**KIND\_DIGIT**](#variable-kind_digit)   = `0`<br> |
|  const uint8\_t | [**KIND\_FLAG**](#variable-kind_flag)   = `2`<br> |
|  const uint8\_t | [**KIND\_GLYPH**](#variable-kind_glyph)   = `1`<br> |
|  const float | [**PX\_PER\_MM**](#variable-px_per_mm)   = `4.35f`<br> |
|  const float | [**SETTLE\_EPS**](#variable-settle_eps)   = `0.02f`<br> |
|  const float | [**SNAP\_LANDING**](#variable-snap_landing)   = `1.5f`<br> |
















## Public Static Functions

| Type | Name |
| ---: | :--- |
|  long | [**pow10l**](#function-pow10l) (uint8\_t n) <br> |


























## Public Types Documentation




### enum DrumFont 

_Glyph font size. Maps to a fixed monospace ProFont face._ 
```C++
enum OpenSkyhawk::DrumFont {
    SMALL = 0,
    LARGE = 1
};
```





**Note:**

SMALL = u8g2\_font\_profont22\_mr, LARGE = u8g2\_font\_profont29\_mr (mono, ASCII). Mono guarantees the flag glyph ('N'/'S'/'E'/'W') is no wider than a digit cell. 





        

<hr>



### enum DrumScroll 

_Scroll behaviour per readout._ 
```C++
enum OpenSkyhawk::DrumScroll {
    EASE_ONLY = 0,
    SNAP_SETTLE = 1
};
```





**Note:**

SNAP\_SETTLE adds the prototype-missing jump handling: deltas above the readout's snapThreshold teleport the tape near the target, then ease the final step, so a sudden 130→250 KIAS change doesn't spin every wheel through 120 intermediate values. 





        

<hr>
## Public Static Attributes Documentation




### variable EASE 

```C++
const float OpenSkyhawk::EASE;
```




<hr>



### variable FRAME\_MS 

```C++
const uint32_t OpenSkyhawk::FRAME_MS;
```




<hr>



### variable KIND\_DIGIT 

```C++
const uint8_t OpenSkyhawk::KIND_DIGIT;
```




<hr>



### variable KIND\_FLAG 

```C++
const uint8_t OpenSkyhawk::KIND_FLAG;
```




<hr>



### variable KIND\_GLYPH 

```C++
const uint8_t OpenSkyhawk::KIND_GLYPH;
```




<hr>



### variable PX\_PER\_MM 

```C++
const float OpenSkyhawk::PX_PER_MM;
```




<hr>



### variable SETTLE\_EPS 

```C++
const float OpenSkyhawk::SETTLE_EPS;
```




<hr>



### variable SNAP\_LANDING 

```C++
const float OpenSkyhawk::SNAP_LANDING;
```




<hr>
## Public Static Functions Documentation




### function pow10l 

```C++
static long OpenSkyhawk::pow10l (
    uint8_t n
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/DrumDisplay/DrumDisplay.cpp`

