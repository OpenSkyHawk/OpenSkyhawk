

# Namespace OpenSkyhawk



[**Namespace List**](namespaces.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md)



_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._
















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**AccelPoint**](structOpenSkyhawk_1_1AccelPoint.md) <br>_One point on the acceleration curve (SwitecX25 form)._  |
| class | [**AnalogInput**](classOpenSkyhawk_1_1AnalogInput.md) <br>_Continuous analog input — one analog_ `PinRef` _, normalised to a 16-bit value 0..65535. Emits the smoothed value over CAN (MULTIPOS transport). Self-registers into_[_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._ |
| class | [**AnalogMultiPos**](classOpenSkyhawk_1_1AnalogMultiPos.md) <br>_Resistor-ladder multi-position selector — one analog_ `PinRef` _, a different voltage per position. Emits the resolved position index 0..N-1 over CAN (MULTIPOS dispatch)._ |
| struct | [**AxisCal**](structOpenSkyhawk_1_1AxisCal.md) <br>_Captured endpoints for one axis, unsigned 0–65535 throughout._  |
| struct | [**CalBlob**](structOpenSkyhawk_1_1CalBlob.md) <br>_The whole persisted calibration set, written and erased as one unit._  |
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
| enum uint8\_t | [**CalNackReason**](#enum-calnackreason)  <br>_NACK reasons._ `detail` _names the offending axis where one applies, else 0xFF._ |
| enum uint8\_t | [**CalType**](#enum-caltype)  <br>_Message types. High bit set = device→client, so direction is readable in a capture._  |
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
|  constexpr uint8\_t | [**AXIS\_CAL\_SLOTS**](#variable-axis_cal_slots)   = `8`<br>_HID report axis slots. Fixed by the report descriptor, not by how many a cockpit populates._  |
|  constexpr uint8\_t | [**CAL\_AXIS\_NONE**](#variable-cal_axis_none)   = `0xFF`<br>`RESET` _and the axis-selection fields use this to mean "all" / "none"._ |
|  constexpr uint16\_t | [**CAL\_ENVELOPE\_BYTES**](#variable-cal_envelope_bytes)   = `10`<br>_magic 4 + type 1 + seq 1 + len 2 + crc 2_  |
|  constexpr uint8\_t | [**CAL\_FRAME\_MAGIC**](#variable-cal_frame_magic)   = `{ 0xAA, 0x53, 0x4B, 0x43 }`<br> |
|  constexpr uint32\_t | [**CAL\_MAGIC**](#variable-cal_magic)   = `/* multi line expression */`<br>_Blob signature. Little-endian in flash, so a hexdump reads "OSKC"._  |
|  constexpr uint16\_t | [**CAL\_MAX\_FRAME**](#variable-cal_max_frame)   = `[**CAL\_ENVELOPE\_BYTES**](namespaceOpenSkyhawk.md#variable-cal_envelope_bytes) + [**CAL\_MAX\_PAYLOAD**](namespaceOpenSkyhawk.md#variable-cal_max_payload)`<br> |
|  constexpr uint16\_t | [**CAL\_MAX\_PAYLOAD**](#variable-cal_max_payload)   = `82`<br>_CAL\_DATA, the largest legal payload._  |
|  constexpr uint8\_t | [**CAL\_PROTO\_VERSION**](#variable-cal_proto_version)   = `1`<br> |
|  constexpr uint16\_t | [**CAL\_VERSION**](#variable-cal_version)   = `1`<br>_Blob layout version._  |
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
|  [**NodeFaultCode**](NodeStatus_8h.md#enum-nodefaultcode) | [**aggregateFaults**](#function-aggregatefaults) (const char \*\* detailOut=nullptr) <br>_Roll up the registered fault sources into a single node fault code (#163)._  |
|  uint16\_t | [**axisCalApply**](#function-axiscalapply) (const [**AxisCal**](structOpenSkyhawk_1_1AxisCal.md) & cal, uint16\_t raw) <br>_Map a raw axis reading through the two-segment calibration._  |
|  bool | [**axisCalValid**](#function-axiscalvalid) (const [**AxisCal**](structOpenSkyhawk_1_1AxisCal.md) & cal) <br>_True when both segments have a non-zero divisor, i.e. the axis is calibrated._  |
|  void | [**calBlobClear**](#function-calblobclear) ([**CalBlob**](structOpenSkyhawk_1_1CalBlob.md) & blob) <br>_Zero a blob so every axis reads as uncalibrated._  |
|  uint16\_t | [**calBlobCrc**](#function-calblobcrc) (const [**CalBlob**](structOpenSkyhawk_1_1CalBlob.md) & blob) <br>_CRC of a blob's covered region, i.e. everything before the_ `crc` _field itself._ |
|  void | [**calBlobSeal**](#function-calblobseal) ([**CalBlob**](structOpenSkyhawk_1_1CalBlob.md) & blob) <br>_Stamp magic, version, and a fresh CRC onto a blob ahead of persisting it._  |
|  bool | [**calBlobValid**](#function-calblobvalid) (const [**CalBlob**](structOpenSkyhawk_1_1CalBlob.md) & blob) <br>_True when a blob carries the right signature, version, and checksum._  |
|  uint16\_t | [**calBuildFrame**](#function-calbuildframe) (uint8\_t \* out, uint16\_t outCap, uint8\_t type, uint8\_t seq, const uint8\_t \* payload, uint16\_t len) <br>_Build a complete frame into a caller-supplied buffer._  |
|  uint16\_t | [**calCrc16**](#function-calcrc16) (const uint8\_t \* data, size\_t len) <br>_CRC-16/CCITT-FALSE — poly 0x1021, init 0xFFFF, no reflection, no final XOR._  |
|  bool | [**calFrameCrcOk**](#function-calframecrcok) (const uint8\_t \* frame, uint16\_t n) <br>_Verify the CRC of a complete, already-assembled frame._  |
|  bool | [**calLenValidForType**](#function-callenvalidfortype) (uint8\_t type, uint16\_t len) <br>_Is_ `len` _the only length this_`type` _may legally carry?_ |
|  [**StepperConfig**](structOpenSkyhawk_1_1StepperConfig.md) | [**makeX27Config**](#function-makex27config) (int16\_t homePosition, int16\_t parkPosition, int16\_t minPos, int16\_t maxPos, [**HomeMode**](namespaceOpenSkyhawk.md#enum-homemode) home=HomeMode::STALL, bool homeSeekClockwise=false, [**HomeSensor**](structOpenSkyhawk_1_1HomeSensor.md) sensor={ true, 5, 2000 }, bool wrap=false, uint8\_t deadband=1, bool autoRecal=false, uint32\_t recalDebounceMs=0, uint16\_t stepsPerRev=1080, uint16\_t rangeSteps=945, uint16\_t homeStepUs=0) <br>_Build a_ [_**StepperConfig**_](structOpenSkyhawk_1_1StepperConfig.md) _with the X27 air-core motor defaults filled in._ |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  long | [**pow10l**](#function-pow10l) (uint8\_t n) <br> |


























## Public Types Documentation




### enum CalNackReason 

_NACK reasons._ `detail` _names the offending axis where one applies, else 0xFF._
```C++
enum OpenSkyhawk::CalNackReason {
    CAL_NACK_BAD_CRC = 0x01,
    CAL_NACK_BAD_LENGTH = 0x02,
    CAL_NACK_BAD_TYPE = 0x03,
    CAL_NACK_BAD_INDEX = 0x04,
    CAL_NACK_BAD_ORDER = 0x05,
    CAL_NACK_NO_SESSION = 0x06,
    CAL_NACK_NO_STORAGE = 0x07,
    CAL_NACK_BAD_DEADZONE = 0x08
};
```




<hr>



### enum CalType 

_Message types. High bit set = device→client, so direction is readable in a capture._ 
```C++
enum OpenSkyhawk::CalType {
    CAL_T_HELLO = 0x01,
    CAL_T_GET_CAL = 0x02,
    CAL_T_SESSION_OPEN = 0x03,
    CAL_T_SESSION_CLOSE = 0x04,
    CAL_T_COMMIT = 0x05,
    CAL_T_RESET = 0x06,
    CAL_T_KEEPALIVE = 0x07,
    CAL_T_STREAM_SELECT = 0x08,
    CAL_T_HELLO_ACK = 0x81,
    CAL_T_CAL_DATA = 0x82,
    CAL_T_SESSION_ACK = 0x83,
    CAL_T_ACK = 0x84,
    CAL_T_NACK = 0x85,
    CAL_T_RAW = 0x86
};
```




<hr>



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




### variable AXIS\_CAL\_SLOTS 

_HID report axis slots. Fixed by the report descriptor, not by how many a cockpit populates._ 
```C++
constexpr uint8_t OpenSkyhawk::AXIS_CAL_SLOTS;
```




<hr>



### variable CAL\_AXIS\_NONE 

`RESET` _and the axis-selection fields use this to mean "all" / "none"._
```C++
constexpr uint8_t OpenSkyhawk::CAL_AXIS_NONE;
```




<hr>



### variable CAL\_ENVELOPE\_BYTES 

_magic 4 + type 1 + seq 1 + len 2 + crc 2_ 
```C++
constexpr uint16_t OpenSkyhawk::CAL_ENVELOPE_BYTES;
```




<hr>



### variable CAL\_FRAME\_MAGIC 

```C++
constexpr uint8_t OpenSkyhawk::CAL_FRAME_MAGIC[4];
```



Frame lead-in. 0xAA leads all non-DCS data on this link, matching the HID frame magic. Distinct from CAL\_MAGIC above, which signs the stored blob rather than a wire frame. 


        

<hr>



### variable CAL\_MAGIC 

_Blob signature. Little-endian in flash, so a hexdump reads "OSKC"._ 
```C++
constexpr uint32_t OpenSkyhawk::CAL_MAGIC;
```




<hr>



### variable CAL\_MAX\_FRAME 

```C++
constexpr uint16_t OpenSkyhawk::CAL_MAX_FRAME;
```




<hr>



### variable CAL\_MAX\_PAYLOAD 

_CAL\_DATA, the largest legal payload._ 
```C++
constexpr uint16_t OpenSkyhawk::CAL_MAX_PAYLOAD;
```




<hr>



### variable CAL\_PROTO\_VERSION 

```C++
constexpr uint8_t OpenSkyhawk::CAL_PROTO_VERSION;
```




<hr>



### variable CAL\_VERSION 

_Blob layout version._ 
```C++
constexpr uint16_t OpenSkyhawk::CAL_VERSION;
```





**Note:**

A mismatch means "absent" — every axis falls back to identity and flash is left untouched until the user commits. No migration and no auto-rewrite: a downgrade-then-upgrade cycle would silently destroy data. 





        

<hr>



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




### function aggregateFaults 

_Roll up the registered fault sources into a single node fault code (#163)._ 
```C++
NodeFaultCode OpenSkyhawk::aggregateFaults (
    const char ** detailOut=nullptr
) 
```



Walks `FaultSource::head()` and returns the **first** source reporting a non-`NONE` `faultCode()`; if `detailOut` is non-null it is set to that source's `faultDetail()`. Returns `NodeFaultCode::NONE` when every source is healthy. `*detailOut` is **always** a non-null string (`""` when healthy or a source returns null) — callers never null-check.




**Note:**

Iteration is **registry order = reverse construction order** (the intrusive list pushes at head), so the last-constructed fault source has priority. With one active fault at a time on the wire this rarely matters; a node with concurrent faults reports the head-most. 




**Note:**

Cheap/const — sources report cached state only. Called on the periodic health path.




**Parameters:**


* `detailOut` Optional out-param for the local DiagSerial detail string (never null on return). 



**Returns:**

The primary NodeFaultCode, or NONE if no source is faulted. 





        

<hr>



### function axisCalApply 

_Map a raw axis reading through the two-segment calibration._ 
```C++
uint16_t OpenSkyhawk::axisCalApply (
    const AxisCal & cal,
    uint16_t raw
) 
```





**Parameters:**


* `cal` Endpoints for this axis. 
* `raw` Unsigned 0–65535 as emitted by the node. 



**Returns:**

Unsigned 0–65535, with `cal.centre` landing exactly on 32768.


Returns `raw` unchanged when the axis is uncalibrated, so an unwritten blob behaves identically to a build without this feature.




**Note:**

Integer only, by design — the arithmetic must be reproducible exactly, and a curve would stack with the per-aircraft curves DCS already applies. 




**Note:**

The uint32\_t cast must sit on the multiply _operand_. Worst case is 65534 × 32768 = 2 147 418 112, inside uint32\_t with 2× headroom. `int` is 32-bit on RP2040 so the wrong form would work here by accident; the explicit cast is the portable contract. 




**Note:**

65535 is reachable only via the upper clamp — the upper segment's arithmetic tops out at 65534. Both map to +32767 after the caller's −32768, so this is correct, but it is the kind of asymmetry someone will otherwise "fix". 





        

<hr>



### function axisCalValid 

_True when both segments have a non-zero divisor, i.e. the axis is calibrated._ 
```C++
bool OpenSkyhawk::axisCalValid (
    const AxisCal & cal
) 
```





**Parameters:**


* `cal` Endpoints to check. 



**Returns:**

true if `min < centre < max` allowing for the deadzone. 




**Note:**

This doubles as the calibrated/uncalibrated predicate — there is no stored validity flag, because a flag could only duplicate or contradict this. An all-zero blob (.bss, never loaded) and an all-0xFF blob (erased flash) both fail it, so both fail closed to identity. 




**Note:**

Widened to uint32\_t deliberately. The obvious `min < centre - deadzone` underflows on uint16\_t when deadzone &gt; centre. The wire protocol rejects a non-zero deadzone, but this guard's whole job is preventing a divide-by-zero in [**axisCalApply()**](namespaceOpenSkyhawk.md#function-axiscalapply), so it must not depend on a check one layer up. 





        

<hr>



### function calBlobClear 

_Zero a blob so every axis reads as uncalibrated._ 
```C++
void OpenSkyhawk::calBlobClear (
    CalBlob & blob
) 
```





**Parameters:**


* `blob` Blob to clear. 



**Note:**

Clears RAM only. Nothing here writes flash. 





        

<hr>



### function calBlobCrc 

_CRC of a blob's covered region, i.e. everything before the_ `crc` _field itself._
```C++
uint16_t OpenSkyhawk::calBlobCrc (
    const CalBlob & blob
) 
```





**Parameters:**


* `blob` Blob to checksum. 



**Returns:**

The CRC to store in, or compare against, `blob.crc`. 





        

<hr>



### function calBlobSeal 

_Stamp magic, version, and a fresh CRC onto a blob ahead of persisting it._ 
```C++
void OpenSkyhawk::calBlobSeal (
    CalBlob & blob
) 
```





**Parameters:**


* `blob` Blob whose `axes` are already populated. 




        

<hr>



### function calBlobValid 

_True when a blob carries the right signature, version, and checksum._ 
```C++
bool OpenSkyhawk::calBlobValid (
    const CalBlob & blob
) 
```





**Parameters:**


* `blob` Blob as read from storage. 



**Returns:**

true if the blob should be trusted. 





        

<hr>



### function calBuildFrame 

_Build a complete frame into a caller-supplied buffer._ 
```C++
uint16_t OpenSkyhawk::calBuildFrame (
    uint8_t * out,
    uint16_t outCap,
    uint8_t type,
    uint8_t seq,
    const uint8_t * payload,
    uint16_t len
) 
```





**Parameters:**


* `out` Destination, at least `CAL_ENVELOPE_BYTES + len` bytes. 
* `outCap` Capacity of `out`. 
* `type` Message type. 
* `seq` Sequence byte — echoed from the request, or a counter for unsolicited RAW. 
* `payload` Payload bytes; may be nullptr when `len` is 0. 
* `len` Payload length. Must satisfy [**calLenValidForType()**](namespaceOpenSkyhawk.md#function-callenvalidfortype). 



**Returns:**

Bytes written, or 0 if the arguments are inconsistent or `out` is too small. 





        

<hr>



### function calCrc16 

_CRC-16/CCITT-FALSE — poly 0x1021, init 0xFFFF, no reflection, no final XOR._ 
```C++
uint16_t OpenSkyhawk::calCrc16 (
    const uint8_t * data,
    size_t len
) 
```





**Parameters:**


* `data` Bytes to cover. 
* `len` Byte count. 



**Returns:**

The CRC. 




**Note:**

Canonical check: "123456789" → 0x29B1. Init is 0xFFFF rather than 0x0000 so that leading zero bytes change the result — an all-zero blob is a realistic corruption mode, and a 0x0000 init would not distinguish it from a shorter all-zero one. Bitwise and table-free: ~70 bytes of input costs a few microseconds, irrelevant beside the ~45 ms sector erase it protects. 





        

<hr>



### function calFrameCrcOk 

_Verify the CRC of a complete, already-assembled frame._ 
```C++
bool OpenSkyhawk::calFrameCrcOk (
    const uint8_t * frame,
    uint16_t n
) 
```





**Parameters:**


* `frame` Whole frame including magic and trailing CRC. 
* `n` Frame length in bytes. 



**Returns:**

true if the trailing CRC matches the computed one. 




**Note:**

Coverage is `TYPE`‖`SEQ`‖`LEN`‖`PAYLOAD` — the magic is excluded, and so is the CRC field itself. Checksumming constant bytes adds no detection power. 





        

<hr>



### function calLenValidForType 

_Is_ `len` _the only length this_`type` _may legally carry?_
```C++
bool OpenSkyhawk::calLenValidForType (
    uint8_t type,
    uint16_t len
) 
```





**Parameters:**


* `type` Message type byte. 
* `len` Candidate payload length, as read off the wire. 



**Returns:**

true if the pair is legal.


This is the framing-layer gate, and it is checked **before the payload is buffered**. `len` is read before the CRC can be verified, so on a false frame it is noise: a stray magic in DCS-BIOS text can decode a length near 65535, and a receiver that waits for that many bytes stalls. Every type therefore has an exact length rather than a shared bound.




**Note:**

There are no variable-length types. `COMMIT` carries exactly one axis, so the rule is uniform: one legal length per type, no exception to state or to get wrong. An earlier draft let `COMMIT` batch up to eight axes, which allowed a batch to name the same axis twice with different values and silently apply the last. 




**Note:**

An unknown type is rejected. Protocol versions must match — `HELLO_ACK` carries `proto` for exactly that — so an unrecognised type is an error, not something to skip. 





        

<hr>



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

