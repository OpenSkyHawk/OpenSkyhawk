

# Namespace OpenSkyhawk



[**Namespace List**](namespaces.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md)



_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._
















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**AccelPoint**](structOpenSkyhawk_1_1AccelPoint.md) <br>_One point on the acceleration curve (SwitecX25 form)._  |
| class | [**AnalogInput**](classOpenSkyhawk_1_1AnalogInput.md) <br>_Continuous analog input — one analog_ `PinRef` _, normalised to a 16-bit value 0..65535. Emits the smoothed value over CAN (MULTIPOS transport). Self-registers into_[_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._ |
| class | [**AnalogMultiPos**](classOpenSkyhawk_1_1AnalogMultiPos.md) <br>_Resistor-ladder multi-position selector — one analog_ `PinRef` _, a different voltage per position. Emits the resolved position index 0..N-1 over CAN (MULTIPOS dispatch)._ |
| class | [**DrumDisplay**](classOpenSkyhawk_1_1DrumDisplay.md) <br>_Rolling-drum OLED readout. One instance == one OLED panel._  |
| struct | [**DrumFlag**](structOpenSkyhawk_1_1DrumFlag.md) <br>_Optional 2-state (or N-state) flag tape — hemisphere N/S · E/W, or a mode letter._  |
| struct | [**DrumGlyph**](structOpenSkyhawk_1_1DrumGlyph.md) <br>_A fixed (non-rolling) glyph painted between digit columns — '.', ' ', ':' etc._  |
| struct | [**DrumReadout**](structOpenSkyhawk_1_1DrumReadout.md) <br>_Complete description of one rolling readout: its sources, geometry, glyphs, flag._  |
| struct | [**DrumSource**](structOpenSkyhawk_1_1DrumSource.md) <br>_One DCS-BIOS digit source feeding a_ [_**DrumReadout**_](structOpenSkyhawk_1_1DrumReadout.md) _._ |
| class | [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md) <br>_A source of node faults — implemented by any object that can fault (#163)._  |
| struct | [**GaugeCal**](structOpenSkyhawk_1_1GaugeCal.md) <br>_Value → position calibration for one gauge._  |
| class | [**HIDAxis**](classOpenSkyhawk_1_1HIDAxis.md) <br>_HID axis handler. Declared at sketch scope for each joystick axis._  |
| class | [**HIDButton**](classOpenSkyhawk_1_1HIDButton.md) <br>_HID button handler. Declared at sketch scope for each button._  |
| class | [**HIDHatSwitch**](classOpenSkyhawk_1_1HIDHatSwitch.md) <br>_HID hat switch handler. Declared at sketch scope for each hat switch._  |
| struct | [**HomeSensor**](structOpenSkyhawk_1_1HomeSensor.md) <br>_Home-sensor parameters (_ HomeMode::SENSOR _only)._ |
| class | [**I2cHealth**](classOpenSkyhawk_1_1I2cHealth.md) <br>_Per-device I2C circuit breaker. Mix into any class that talks to an I2C device._  |
| class | [**I2cMux**](classOpenSkyhawk_1_1I2cMux.md) <br>_Selects one downstream channel of a TCA9548A I2C multiplexer._  |
| class | [**InputBase**](classOpenSkyhawk_1_1InputBase.md) <br>_Abstract base for all hardware-polled input objects._  |
| class | [**LED**](classOpenSkyhawk_1_1LED.md) <br>_Digital_ [_**LED**_](classOpenSkyhawk_1_1LED.md) _output. Drives a pin based on a DCS-BIOS state value._ |
| class | [**MotorDriver**](classOpenSkyhawk_1_1MotorDriver.md) <br>_Common interface every motor/servo backend implements._  |
| class | [**MultiPosInput**](classOpenSkyhawk_1_1MultiPosInput.md) <br>_Base for the MULTIPOS input family — selectors that emit an absolute position index 0..N-1 over CAN. Self-registers into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._ |
| class | [**NeedleGauge**](classOpenSkyhawk_1_1NeedleGauge.md) <br>_DCS-driven pointer gauge over any_ [_**MotorDriver**_](classOpenSkyhawk_1_1MotorDriver.md) _backend._ |
| class | [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) <br>_Abstract base for all DCS-driven output objects._  |
| class | [**RotaryEncoder**](classOpenSkyhawk_1_1RotaryEncoder.md) <br>_Incremental quadrature encoder on two pins (A/B). Emits a signed_ **relative** _value per detent over CAN — direction in the sign, magnitude set by the mode. Self-registers into_[_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._ |
| class | [**ShiftBus**](classOpenSkyhawk_1_1ShiftBus.md) <br>_One shared SPI shift-register bus ('165 inputs + '595 outputs)._  |
| struct | [**StepperConfig**](structOpenSkyhawk_1_1StepperConfig.md) <br>_Full per-instance stepper configuration. Authored per sketch (panel wiring)._  |
| class | [**StepperMotor**](classOpenSkyhawk_1_1StepperMotor.md) <br>_Non-blocking instrument-gauge stepper driven through_ [_**PinRef**_](classPinRef.md) _coils._ |
| class | [**Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md) <br>_Debounced 2-position switch. Self-registers into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._ |
| class | [**Switch3Pos**](classOpenSkyhawk_1_1Switch3Pos.md) <br>_Three-position switch (ON-OFF-ON / spring-centred) on two pins. Emits 0 / 1 / 2 over CAN (MULTIPOS dispatch)._  |
| class | [**SwitchMultiPos**](classOpenSkyhawk_1_1SwitchMultiPos.md) <br>_Multi-position rotary selector — N discrete pins, exactly one active at a time. Emits the active position index 0..N-1 over CAN (MULTIPOS dispatch)._  |


## Public Types

| Type | Name |
| ---: | :--- |
| enum uint8\_t | [**DrumFont**](#enum-drumfont)  <br>_Glyph font size. Maps to a fixed monospace ProFont face._  |
| enum uint8\_t | [**DrumScroll**](#enum-drumscroll)  <br>_Scroll behaviour per readout._  |
| enum uint8\_t | [**EncoderMode**](#enum-encodermode)  <br>_Relative-dispatch mode — picks the DCS-BIOS interface the bridge drives, hence the CAN frame + payload encoding this encoder uses per detent. Scoped enum._  |
| enum uint8\_t | [**EncoderStepsPerDetent**](#enum-encoderstepsperdetent)  <br>_Quadrature transitions per mechanical detent (match to the encoder). Scoped enum._  |
| enum uint8\_t | [**HomeMode**](#enum-homemode)  <br>_How the driver establishes its zero reference at boot._  |
| enum uint8\_t | [**LeadingZero**](#enum-leadingzero)  <br>_Leading-zero handling for a readout's high-order digit cells._  |
| enum uint8\_t | [**StepPattern**](#enum-steppattern)  <br>_Coil energising sequence._  |




## Public Attributes

| Type | Name |
| ---: | :--- |
|  const [**AccelPoint**](structOpenSkyhawk_1_1AccelPoint.md) | [**kSwitecDefaultAccel**](#variable-kswitecdefaultaccel)   = `/* multi line expression */`<br>_Default SwitecX25 acceleration table; fits the X27/VID-29/BKA-30 air-core family._  |
|  constexpr uint8\_t | [**kSwitecDefaultAccelN**](#variable-kswitecdefaultacceln)   = `5`<br> |


## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**ANALOG\_NC**](#variable-analog_nc)   = `[**MultiPosInput::NO\_POSITION**](classOpenSkyhawk_1_1MultiPosInput.md#variable-no_position)`<br>`posVals[]` _sentinel: a position with no physical detent (no distinct voltage)._ |
|  const float | [**EASE**](#variable-ease)   = `0.30f`<br> |
|  const uint32\_t | [**FRAME\_MS**](#variable-frame_ms)   = `16`<br> |
|  const uint8\_t | [**KIND\_DIGIT**](#variable-kind_digit)   = `0`<br> |
|  const uint8\_t | [**KIND\_FLAG**](#variable-kind_flag)   = `2`<br> |
|  const uint8\_t | [**KIND\_GLYPH**](#variable-kind_glyph)   = `1`<br> |
|  const float | [**PX\_PER\_MM**](#variable-px_per_mm)   = `4.35f`<br> |
|  const float | [**SETTLE\_EPS**](#variable-settle_eps)   = `0.02f`<br> |
|  const float | [**SNAP\_LANDING**](#variable-snap_landing)   = `1.5f`<br> |














## Public Functions

| Type | Name |
| ---: | :--- |
|  [**StepperConfig**](structOpenSkyhawk_1_1StepperConfig.md) | [**makeX27Config**](#function-makex27config) (int16\_t homePosition, int16\_t parkPosition, int16\_t minPos, int16\_t maxPos, [**HomeMode**](namespaceOpenSkyhawk.md#enum-homemode) home=HomeMode::STALL, bool homeSeekClockwise=false, [**HomeSensor**](structOpenSkyhawk_1_1HomeSensor.md) sensor={ true, 5, 2000 }, bool wrap=false, uint8\_t deadband=1, bool autoRecal=false, uint32\_t recalDebounceMs=0, uint16\_t stepsPerRev=1080, uint16\_t rangeSteps=945, uint16\_t homeStepUs=0) <br>_Build a_ [_**StepperConfig**_](structOpenSkyhawk_1_1StepperConfig.md) _with the X27 air-core motor defaults filled in._ |


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



### enum EncoderMode 

_Relative-dispatch mode — picks the DCS-BIOS interface the bridge drives, hence the CAN frame + payload encoding this encoder uses per detent. Scoped enum._ 
```C++
enum OpenSkyhawk::EncoderMode {
    Rel,
    Dir
};
```




<hr>



### enum EncoderStepsPerDetent 

_Quadrature transitions per mechanical detent (match to the encoder). Scoped enum._ 
```C++
enum OpenSkyhawk::EncoderStepsPerDetent {
    One = 1,
    Two = 2,
    Four = 4,
    Eight = 8
};
```




<hr>



### enum HomeMode 

_How the driver establishes its zero reference at boot._ 
```C++
enum OpenSkyhawk::HomeMode {
    STALL,
    SENSOR
};
```




<hr>



### enum LeadingZero 

_Leading-zero handling for a readout's high-order digit cells._ 
```C++
enum OpenSkyhawk::LeadingZero {
    Keep = 0,
    Suppress = 1
};
```





**Note:**

Suppress blanks the high-order zero cells down to the target's significant-digit count (units always shows, so 0 renders "0"); animation is unchanged. Keep is fixed width. 





        

<hr>



### enum StepPattern 

_Coil energising sequence._ 
```C++
enum OpenSkyhawk::StepPattern {
    SWITEC_6STATE,
    FULL_4STATE
};
```




<hr>
## Public Attributes Documentation




### variable kSwitecDefaultAccel 

_Default SwitecX25 acceleration table; fits the X27/VID-29/BKA-30 air-core family._ 
```C++
const AccelPoint OpenSkyhawk::kSwitecDefaultAccel;
```




<hr>



### variable kSwitecDefaultAccelN 

```C++
constexpr uint8_t OpenSkyhawk::kSwitecDefaultAccelN;
```




<hr>
## Public Static Attributes Documentation




### variable ANALOG\_NC 

`posVals[]` _sentinel: a position with no physical detent (no distinct voltage)._
```C++
constexpr uint16_t OpenSkyhawk::ANALOG_NC;
```



The uint16\_t analog of `SwitchMultiPos`'s `PIN_NC` — same "this position index has no physical
input" role, but a different sentinel because an analog ladder is an array of ADC _values_ (uint16\_t), not `PinRef`s. Kept `==  MultiPosInput::NO_POSITION` (both 0xFFFF) so there is one sentinel value across the MULTIPOS family. 


        

<hr>



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
## Public Functions Documentation




### function makeX27Config 

_Build a_ [_**StepperConfig**_](structOpenSkyhawk_1_1StepperConfig.md) _with the X27 air-core motor defaults filled in._
```C++
StepperConfig OpenSkyhawk::makeX27Config (
    int16_t homePosition,
    int16_t parkPosition,
    int16_t minPos,
    int16_t maxPos,
    HomeMode home=HomeMode::STALL,
    bool homeSeekClockwise=false,
    HomeSensor sensor={ true, 5, 2000 },
    bool wrap=false,
    uint8_t deadband=1,
    bool autoRecal=false,
    uint32_t recalDebounceMs=0,
    uint16_t stepsPerRev=1080,
    uint16_t rangeSteps=945,
    uint16_t homeStepUs=0
) 
```



Bakes the motor-invariant fields — `stepsPerRev`, `pattern` (SWITEC\_6STATE), and the default SwitecX25 accel table — so a sketch specifies only the per-gauge wiring/travel. Shared by every X27 / VID-29 / BKA-30 gauge; override any default for a specific panel.




**Parameters:**


* `homePosition` step index at the home reference. 
* `parkPosition` rest position after homing. 
* `minPos` lower moveTo travel clamp (ignored if wrap). 
* `maxPos` upper moveTo travel clamp (ignored if wrap). 
* `home` homing strategy. Default STALL. 
* `homeSeekClockwise` seek direction. Default false. 
* `sensor` home-sensor params (SENSOR mode). Default active-low, 5 ms, 2000 steps. 
* `wrap` continuous-rotation gauge. Default false. 
* `deadband` anti-jitter band, steps. Default 1. 
* `autoRecal` re-zero on sensor crossing. Default false. 
* `recalDebounceMs` minimum interval between auto-recals. Default 0. 
* `stepsPerRev` full revolution in steps. Default 1080 (X27/BKA datasheet, 1/3°/step). 
* `rangeSteps` mechanical stop-to-stop travel in steps = STALL home distance. Default 945 (X27.589 ~315°); set per gauge (e.g. 960 for a 320° BKA-30). 
* `homeStepUs` homing seek rate µs/step. Default 0 → library default (2000 ≈ 500 steps/s). Keep under the motor start-stop rate (~774 steps/s) or the seek slips. 



**Returns:**

Populated [**StepperConfig**](structOpenSkyhawk_1_1StepperConfig.md). 





        

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

