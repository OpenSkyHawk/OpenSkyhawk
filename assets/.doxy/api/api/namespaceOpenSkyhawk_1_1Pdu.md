

# Namespace OpenSkyhawk::Pdu



[**Namespace List**](namespaces.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**Pdu**](namespaceOpenSkyhawk_1_1Pdu.md)




















## Classes

| Type | Name |
| ---: | :--- |
| class | [**CalStore**](classOpenSkyhawk_1_1Pdu_1_1CalStore.md) <br>_The reserved flash page holding the calibration log._  |
| struct | [**CalValues**](structOpenSkyhawk_1_1Pdu_1_1CalValues.md) <br>_Calibration values the conversions need. Defaults = uncalibrated._  |
| struct | [**Latch**](structOpenSkyhawk_1_1Pdu_1_1Latch.md) <br>_A condition that must hold_ `setMs` _to raise and its clear condition hold_`clearMs` _to drop._ |
| class | [**TelemSchedule**](classOpenSkyhawk_1_1Pdu_1_1TelemSchedule.md) <br> |


## Public Types

| Type | Name |
| ---: | :--- |
| enum uint8\_t | [**CalFlag**](#enum-calflag)  <br>_CalRecord.calFlags bits — which calibrations this record holds._  |






## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint32\_t | [**ADC\_FULL**](#variable-adc_full)   = `1UL &lt;&lt; 16`<br>_analogReadResolution(16), set by_ [_**STM32Board**_](namespaceSTM32Board.md) __ |
|  constexpr uint32\_t | [**CAL\_MAGIC**](#variable-cal_magic)   = `0x4C414350`<br>_'PCAL' little-endian_  |
|  constexpr uint32\_t | [**CAL\_PAGE\_SIZE**](#variable-cal_page_size)   = `1024`<br> |
|  constexpr uint8\_t | [**CAL\_SLOTS**](#variable-cal_slots)   = `[**CAL\_PAGE\_SIZE**](namespaceOpenSkyhawk_1_1Pdu.md#variable-cal_page_size) / sizeof(CalRecord)`<br> |
|  constexpr uint8\_t | [**CAL\_VERSION**](#variable-cal_version)   = `1`<br> |
|  constexpr uint16\_t | [**GAIN\_UNITY**](#variable-gain_unity)   = `10000`<br>_cal gain trims: 10000 = ×1.0000_  |
|  constexpr uint32\_t | [**I\_MA\_PER\_MV**](#variable-i_ma_per_mv)   = `2`<br>_INA180A2 (gain 50 V/V) on a 10 mΩ shunt: I = Vout / (50 × 0.010 Ω) → mA = 2 × mV._  |
|  constexpr float | [**NTC\_BETA\_K**](#variable-ntc_beta_k)   = `3450.0f`<br> |
|  constexpr float | [**NTC\_BIAS\_OHM**](#variable-ntc_bias_ohm)   = `10000.0f`<br> |
|  constexpr float | [**NTC\_R0\_OHM**](#variable-ntc_r0_ohm)   = `10000.0f`<br>_NTC: 10 kΩ ±1 % B3450 to GND, 10 kΩ bias from +3V3 (same rail as VDDA → ratiometric)._  |
|  constexpr float | [**NTC\_T0\_K**](#variable-ntc_t0_k)   = `298.15f`<br> |
|  constexpr uint32\_t | [**V12\_DIV\_DEN**](#variable-v12_div_den)   = `10`<br> |
|  constexpr uint32\_t | [**V12\_DIV\_NUM**](#variable-v12_div_num)   = `43`<br>_Rail = ADC × num / den. 12 V: (33k + 10k) / 10k. 5 V: (10k + 13k) / 13k._  |
|  constexpr uint32\_t | [**V5\_DIV\_DEN**](#variable-v5_div_den)   = `13`<br> |
|  constexpr uint32\_t | [**V5\_DIV\_NUM**](#variable-v5_div_num)   = `23`<br> |
|  constexpr uint16\_t | [**VDDA\_NOMINAL\_MV**](#variable-vdda_nominal_mv)   = `3300`<br>_assumed VDDA until CAL VDDA anchors Vrefint_  |
|  constexpr uint16\_t | [**VREFINT\_TYP\_MV**](#variable-vrefint_typ_mv)   = `1200`<br>_F103 Vrefint typical (untrimmed, ±~3 %)_  |














## Public Functions

| Type | Name |
| ---: | :--- |
|  struct | [**\_\_attribute\_\_**](#function-__attribute__) ((packed)) <br>_One stored calibration — 32 bytes, little-endian, CRC-32 over bytes 0–27._  |
|  CalRecord | [**calDefaults**](#function-caldefaults) () <br>_An uncalibrated record: unity gains, zero offsets, no flags._  |
|  void | [**calSeal**](#function-calseal) (CalRecord & rec) <br>_Fill magic/version/crc for_ `rec` _(seq left as set)._ |
|  bool | [**calValid**](#function-calvalid) (const CalRecord & rec) <br>_True when_ `rec` _has the right magic, version and CRC._ |
|  [**CalValues**](structOpenSkyhawk_1_1Pdu_1_1CalValues.md) | [**calValues**](#function-calvalues) (const CalRecord & rec) <br>_The conversion values a record carries._  |
|  int16\_t | [**clampI16**](#function-clampi16) (int32\_t v) <br>_Clamp an int32 into int16 range._  |
|  uint16\_t | [**clampU16**](#function-clampu16) (int32\_t v) <br>_Clamp an int32 into uint16 range._  |
|  uint32\_t | [**countsToMv**](#function-countstomv) (uint16\_t counts, uint16\_t vddaMv) <br>_ADC pin voltage in mV for a 16-bit count at the given VDDA._  |
|  uint32\_t | [**crc32**](#function-crc32) (const uint8\_t \* data, size\_t len) <br>_CRC-32 (IEEE 802.3, reflected 0xEDB88320) over_ `len` _bytes._ |
|  int32\_t | [**currentMa**](#function-currentma) (uint32\_t adcMv, int16\_t zeroDmA, uint16\_t iGain) <br>_Rail current in mA from the INA180 output voltage._  |
|  int16\_t | [**ntcTempDC**](#function-ntctempdc) (uint16\_t counts, int16\_t offsetDC, bool & fault) <br>_Board temperature in 0.1 °C from the NTC channel (ratiometric — VDDA cancels)._  |
|  void | [**putLe16**](#function-putle16) (uint8\_t \* p, uint16\_t v) <br>_Store_ `v` _little-endian at_`p[0..1]` _— a multi-byte TELEM page field._ |
|  uint16\_t | [**rail12Mv**](#function-rail12mv) (uint32\_t adcMv, uint16\_t vGain) <br>_12 V rail in mV from the ADC pin voltage, after the divider and gain trim._  |
|  uint16\_t | [**rail5Mv**](#function-rail5mv) (uint32\_t adcMv, uint16\_t vGain) <br>_5 V rail in mV from the ADC pin voltage, after the divider and gain trim._  |
|  bool | [**railed**](#function-railed) (uint16\_t counts) <br>_True when a 16-bit count is at the top rail (≥ 99.5 % FS) — the channel can't measure._  |
|  int32\_t | [**rawCurrentDmA**](#function-rawcurrentdma) (uint32\_t adcMv) <br>_Uncorrected current in 0.1 mA (no zero, unity gain) — what CAL ZERO measures._  |
|  uint16\_t | [**vddaMv**](#function-vddamv) (uint16\_t vrefintRaw, uint16\_t vrefintMv) <br>_VDDA in mV from a raw Vrefint reading._  |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  bool | [**slotEmpty**](#function-slotempty) (const CalRecord \* s) <br> |
|  uint16\_t | [**trimmed**](#function-trimmed) (uint32\_t mv, uint16\_t gain) <br> |


























## Public Types Documentation




### enum CalFlag 

_CalRecord.calFlags bits — which calibrations this record holds._ 
```C++
enum OpenSkyhawk::Pdu::CalFlag {
    CAL_VDDA = 0x01,
    CAL_ZERO = 0x02,
    CAL_GAIN = 0x04,
    CAL_NTC = 0x08
};
```




<hr>
## Public Static Attributes Documentation




### variable ADC\_FULL 

_analogReadResolution(16), set by_ [_**STM32Board**_](namespaceSTM32Board.md) __
```C++
constexpr uint32_t OpenSkyhawk::Pdu::ADC_FULL;
```




<hr>



### variable CAL\_MAGIC 

_'PCAL' little-endian_ 
```C++
constexpr uint32_t OpenSkyhawk::Pdu::CAL_MAGIC;
```




<hr>



### variable CAL\_PAGE\_SIZE 

```C++
constexpr uint32_t OpenSkyhawk::Pdu::CAL_PAGE_SIZE;
```




<hr>



### variable CAL\_SLOTS 

```C++
constexpr uint8_t OpenSkyhawk::Pdu::CAL_SLOTS;
```




<hr>



### variable CAL\_VERSION 

```C++
constexpr uint8_t OpenSkyhawk::Pdu::CAL_VERSION;
```




<hr>



### variable GAIN\_UNITY 

_cal gain trims: 10000 = ×1.0000_ 
```C++
constexpr uint16_t OpenSkyhawk::Pdu::GAIN_UNITY;
```




<hr>



### variable I\_MA\_PER\_MV 

_INA180A2 (gain 50 V/V) on a 10 mΩ shunt: I = Vout / (50 × 0.010 Ω) → mA = 2 × mV._ 
```C++
constexpr uint32_t OpenSkyhawk::Pdu::I_MA_PER_MV;
```




<hr>



### variable NTC\_BETA\_K 

```C++
constexpr float OpenSkyhawk::Pdu::NTC_BETA_K;
```




<hr>



### variable NTC\_BIAS\_OHM 

```C++
constexpr float OpenSkyhawk::Pdu::NTC_BIAS_OHM;
```




<hr>



### variable NTC\_R0\_OHM 

_NTC: 10 kΩ ±1 % B3450 to GND, 10 kΩ bias from +3V3 (same rail as VDDA → ratiometric)._ 
```C++
constexpr float OpenSkyhawk::Pdu::NTC_R0_OHM;
```




<hr>



### variable NTC\_T0\_K 

```C++
constexpr float OpenSkyhawk::Pdu::NTC_T0_K;
```




<hr>



### variable V12\_DIV\_DEN 

```C++
constexpr uint32_t OpenSkyhawk::Pdu::V12_DIV_DEN;
```




<hr>



### variable V12\_DIV\_NUM 

_Rail = ADC × num / den. 12 V: (33k + 10k) / 10k. 5 V: (10k + 13k) / 13k._ 
```C++
constexpr uint32_t OpenSkyhawk::Pdu::V12_DIV_NUM;
```




<hr>



### variable V5\_DIV\_DEN 

```C++
constexpr uint32_t OpenSkyhawk::Pdu::V5_DIV_DEN;
```




<hr>



### variable V5\_DIV\_NUM 

```C++
constexpr uint32_t OpenSkyhawk::Pdu::V5_DIV_NUM;
```




<hr>



### variable VDDA\_NOMINAL\_MV 

_assumed VDDA until CAL VDDA anchors Vrefint_ 
```C++
constexpr uint16_t OpenSkyhawk::Pdu::VDDA_NOMINAL_MV;
```




<hr>



### variable VREFINT\_TYP\_MV 

_F103 Vrefint typical (untrimmed, ±~3 %)_ 
```C++
constexpr uint16_t OpenSkyhawk::Pdu::VREFINT_TYP_MV;
```




<hr>
## Public Functions Documentation




### function \_\_attribute\_\_ 

_One stored calibration — 32 bytes, little-endian, CRC-32 over bytes 0–27._ 
```C++
struct OpenSkyhawk::Pdu::__attribute__ (
    (packed)
) 
```




<hr>



### function calDefaults 

_An uncalibrated record: unity gains, zero offsets, no flags._ 
```C++
CalRecord OpenSkyhawk::Pdu::calDefaults () 
```




<hr>



### function calSeal 

_Fill magic/version/crc for_ `rec` _(seq left as set)._
```C++
void OpenSkyhawk::Pdu::calSeal (
    CalRecord & rec
) 
```




<hr>



### function calValid 

_True when_ `rec` _has the right magic, version and CRC._
```C++
bool OpenSkyhawk::Pdu::calValid (
    const CalRecord & rec
) 
```




<hr>



### function calValues 

_The conversion values a record carries._ 
```C++
CalValues OpenSkyhawk::Pdu::calValues (
    const CalRecord & rec
) 
```




<hr>



### function clampI16 

_Clamp an int32 into int16 range._ 
```C++
int16_t OpenSkyhawk::Pdu::clampI16 (
    int32_t v
) 
```




<hr>



### function clampU16 

_Clamp an int32 into uint16 range._ 
```C++
uint16_t OpenSkyhawk::Pdu::clampU16 (
    int32_t v
) 
```




<hr>



### function countsToMv 

_ADC pin voltage in mV for a 16-bit count at the given VDDA._ 
```C++
uint32_t OpenSkyhawk::Pdu::countsToMv (
    uint16_t counts,
    uint16_t vddaMv
) 
```




<hr>



### function crc32 

_CRC-32 (IEEE 802.3, reflected 0xEDB88320) over_ `len` _bytes._
```C++
uint32_t OpenSkyhawk::Pdu::crc32 (
    const uint8_t * data,
    size_t len
) 
```




<hr>



### function currentMa 

_Rail current in mA from the INA180 output voltage._ 
```C++
int32_t OpenSkyhawk::Pdu::currentMa (
    uint32_t adcMv,
    int16_t zeroDmA,
    uint16_t iGain
) 
```





**Parameters:**


* `adcMv` INA180 output at the ADC pin, mV. 
* `zeroDmA` Zero offset to subtract, 0.1 mA (from CAL ZERO). 
* `iGain` Gain trim, 10000 = ×1. 



**Returns:**

Signed mA — a small negative value is residual offset, kept so it stays visible. 





        

<hr>



### function ntcTempDC 

_Board temperature in 0.1 °C from the NTC channel (ratiometric — VDDA cancels)._ 
```C++
int16_t OpenSkyhawk::Pdu::ntcTempDC (
    uint16_t counts,
    int16_t offsetDC,
    bool & fault
) 
```





**Parameters:**


* `counts` 16-bit ADC count of the NTC node. 
* `offsetDC` One-point offset, 0.1 °C. 
* `fault` Set true when the reading is outside 2–98 % of full scale (open / shorted NTC). 



**Returns:**

Temperature in 0.1 °C, or NTC\_TEMP\_UNAVAILABLE ([**NodeStatus.h**](NodeStatus_8h.md)) when `fault`. 





        

<hr>



### function putLe16 

_Store_ `v` _little-endian at_`p[0..1]` _— a multi-byte TELEM page field._
```C++
inline void OpenSkyhawk::Pdu::putLe16 (
    uint8_t * p,
    uint16_t v
) 
```




<hr>



### function rail12Mv 

_12 V rail in mV from the ADC pin voltage, after the divider and gain trim._ 
```C++
uint16_t OpenSkyhawk::Pdu::rail12Mv (
    uint32_t adcMv,
    uint16_t vGain
) 
```




<hr>



### function rail5Mv 

_5 V rail in mV from the ADC pin voltage, after the divider and gain trim._ 
```C++
uint16_t OpenSkyhawk::Pdu::rail5Mv (
    uint32_t adcMv,
    uint16_t vGain
) 
```




<hr>



### function railed 

_True when a 16-bit count is at the top rail (≥ 99.5 % FS) — the channel can't measure._ 
```C++
bool OpenSkyhawk::Pdu::railed (
    uint16_t counts
) 
```




<hr>



### function rawCurrentDmA 

_Uncorrected current in 0.1 mA (no zero, unity gain) — what CAL ZERO measures._ 
```C++
int32_t OpenSkyhawk::Pdu::rawCurrentDmA (
    uint32_t adcMv
) 
```




<hr>



### function vddaMv 

_VDDA in mV from a raw Vrefint reading._ 
```C++
uint16_t OpenSkyhawk::Pdu::vddaMv (
    uint16_t vrefintRaw,
    uint16_t vrefintMv
) 
```





**Parameters:**


* `vrefintRaw` 16-bit ADC count of the internal reference (0 = unreadable). 
* `vrefintMv` This chip's calibrated Vrefint, or 0 when uncalibrated. 



**Returns:**

VDDA in mV: Vrefint-tracked when calibrated, VDDA\_NOMINAL\_MV otherwise (the untrimmed F103 Vrefint is a worse reference than the regulator's nominal until anchored). 





        

<hr>
## Public Static Functions Documentation




### function slotEmpty 

```C++
static bool OpenSkyhawk::Pdu::slotEmpty (
    const CalRecord * s
) 
```




<hr>



### function trimmed 

```C++
static uint16_t OpenSkyhawk::Pdu::trimmed (
    uint32_t mv,
    uint16_t gain
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/PduCalStore.cpp`

