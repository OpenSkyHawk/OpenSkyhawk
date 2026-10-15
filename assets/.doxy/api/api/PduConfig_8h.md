

# File PduConfig.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**PduConfig.h**](PduConfig_8h.md)

[Go to the source code of this file](PduConfig_8h_source.md)

_PDU monitoring node — hardware constants and tunable thresholds (#216)._ [More...](#detailed-description)

* `#include <stdint.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |
| namespace | [**Pdu**](namespaceOpenSkyhawk_1_1Pdu.md) <br> |



















































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**PDU\_12V\_OC\_MA**](PduConfig_8h.md#define-pdu_12v_oc_ma)  `4500`<br>_12 V over-current — 90 % of the 5 A fuse_  |
| define  | [**PDU\_12V\_OV\_MV**](PduConfig_8h.md#define-pdu_12v_ov_mv)  `13200`<br>_12 V over-voltage (below the SMBJ12A clamp)_  |
| define  | [**PDU\_12V\_UV\_MV**](PduConfig_8h.md#define-pdu_12v_uv_mv)  `11400`<br>_12 V under-voltage (ATX −5 %)_  |
| define  | [**PDU\_5V\_OC\_MA**](PduConfig_8h.md#define-pdu_5v_oc_ma)  `1800`<br>_5 V over-current — 90 % of the 2 A fuse_  |
| define  | [**PDU\_5V\_OV\_MV**](PduConfig_8h.md#define-pdu_5v_ov_mv)  `5500`<br>_5 V over-voltage (below the SMBJ6.0A standoff)_  |
| define  | [**PDU\_5V\_UV\_MV**](PduConfig_8h.md#define-pdu_5v_uv_mv)  `4750`<br>_5 V under-voltage (−5 %)_  |
| define  | [**PDU\_BOOT\_BLANK\_MS**](PduConfig_8h.md#define-pdu_boot_blank_ms)  `500`<br>_ignore rail/thermal faults this long after boot (ramp + inrush)_  |
| define  | [**PDU\_CAL\_PAGE\_ADDR**](PduConfig_8h.md#define-pdu_cal_page_addr)  `0x0800FC00UL`<br>_last 1 KB page of the STM32F103C8 (64 KB) — keep out of the image_  |
| define  | [**PDU\_CLEAR\_MS**](PduConfig_8h.md#define-pdu_clear_ms)  `500`<br>_a condition must be gone this long to clear_  |
| define  | [**PDU\_FUSE\_OPEN\_MV**](PduConfig_8h.md#define-pdu_fuse_open_mv)  `1000`<br>_rail below this while the node runs → fuse open (12 V only)_  |
| define  | [**PDU\_OC\_SET\_MS**](PduConfig_8h.md#define-pdu_oc_set_ms)  `100`<br>_over-current must persist this long to set_  |
| define  | [**PDU\_PIN\_I12**](PduConfig_8h.md#define-pdu_pin_i12)  `PA2`<br>_I\_12V\_ADC — INA180A2 output._  |
| define  | [**PDU\_PIN\_I5**](PduConfig_8h.md#define-pdu_pin_i5)  `PA3`<br>_I\_5V\_ADC — INA180A2 output._  |
| define  | [**PDU\_PIN\_NTC**](PduConfig_8h.md#define-pdu_pin_ntc)  `PA6`<br>_NTC\_ADC — 10k from +3V3, NTC to GND._  |
| define  | [**PDU\_PIN\_V12**](PduConfig_8h.md#define-pdu_pin_v12)  `PA0`<br>_12V\_READ\_ADC — 33k/10k divider_  |
| define  | [**PDU\_PIN\_V5**](PduConfig_8h.md#define-pdu_pin_v5)  `PA1`<br>_5V\_READ\_ADC — 10k/13k divider_  |
| define  | [**PDU\_SAMPLE\_US**](PduConfig_8h.md#define-pdu_sample_us)  `1000`<br>_rail scan period (1 kHz target)_  |
| define  | [**PDU\_TEMP\_CRIT\_DC**](PduConfig_8h.md#define-pdu_temp_crit_dc)  `750`<br>_board temperature critical, 75.0 °C_  |
| define  | [**PDU\_TEMP\_HYST\_DC**](PduConfig_8h.md#define-pdu_temp_hyst_dc)  `30`<br>_temperature-threshold hysteresis, 3.0 °C_  |
| define  | [**PDU\_TEMP\_WARN\_DC**](PduConfig_8h.md#define-pdu_temp_warn_dc)  `600`<br>_board temperature warning, 60.0 °C_  |
| define  | [**PDU\_V\_HYST\_MV**](PduConfig_8h.md#define-pdu_v_hyst_mv)  `100`<br>_voltage-threshold hysteresis_  |
| define  | [**PDU\_V\_SET\_MS**](PduConfig_8h.md#define-pdu_v_set_ms)  `50`<br>_a voltage condition must persist this long to set_  |
| define  | [**PDU\_ZERO\_GUARD\_12V\_MA**](PduConfig_8h.md#define-pdu_zero_guard_12v_ma)  `75`<br>_CAL ZERO refuses if the 12 V raw current exceeds this (load present)_  |
| define  | [**PDU\_ZERO\_GUARD\_5V\_MA**](PduConfig_8h.md#define-pdu_zero_guard_5v_ma)  `120`<br>_5 V guard — higher: the PDU's own ~30–50 mA flows through this shunt_  |

## Detailed Description


Hardware constants match the PDU Rev 0.2.0 schematic (`PCB/Base/Power/PDU`, #202): pin map, divider ratios, INA180A2 gain and shunt, NTC bias. Every threshold is a `PDU_*` macro with a default, so a build overrides it with `build_flags = -DPDU_12V_UV_MV=11200` without a code change; the defaults are starting points to be tuned on the bench against the real load.


Units: mV, mA, ms, and tenths of a degree C (`DC`).




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    
## Macro Definition Documentation





### define PDU\_12V\_OC\_MA 

_12 V over-current — 90 % of the 5 A fuse_ 
```C++
#define PDU_12V_OC_MA `4500`
```




<hr>



### define PDU\_12V\_OV\_MV 

_12 V over-voltage (below the SMBJ12A clamp)_ 
```C++
#define PDU_12V_OV_MV `13200`
```




<hr>



### define PDU\_12V\_UV\_MV 

_12 V under-voltage (ATX −5 %)_ 
```C++
#define PDU_12V_UV_MV `11400`
```




<hr>



### define PDU\_5V\_OC\_MA 

_5 V over-current — 90 % of the 2 A fuse_ 
```C++
#define PDU_5V_OC_MA `1800`
```




<hr>



### define PDU\_5V\_OV\_MV 

_5 V over-voltage (below the SMBJ6.0A standoff)_ 
```C++
#define PDU_5V_OV_MV `5500`
```




<hr>



### define PDU\_5V\_UV\_MV 

_5 V under-voltage (−5 %)_ 
```C++
#define PDU_5V_UV_MV `4750`
```




<hr>



### define PDU\_BOOT\_BLANK\_MS 

_ignore rail/thermal faults this long after boot (ramp + inrush)_ 
```C++
#define PDU_BOOT_BLANK_MS `500`
```




<hr>



### define PDU\_CAL\_PAGE\_ADDR 

_last 1 KB page of the STM32F103C8 (64 KB) — keep out of the image_ 
```C++
#define PDU_CAL_PAGE_ADDR `0x0800FC00UL`
```




<hr>



### define PDU\_CLEAR\_MS 

_a condition must be gone this long to clear_ 
```C++
#define PDU_CLEAR_MS `500`
```




<hr>



### define PDU\_FUSE\_OPEN\_MV 

_rail below this while the node runs → fuse open (12 V only)_ 
```C++
#define PDU_FUSE_OPEN_MV `1000`
```




<hr>



### define PDU\_OC\_SET\_MS 

_over-current must persist this long to set_ 
```C++
#define PDU_OC_SET_MS `100`
```




<hr>



### define PDU\_PIN\_I12 

_I\_12V\_ADC — INA180A2 output._ 
```C++
#define PDU_PIN_I12 `PA2`
```




<hr>



### define PDU\_PIN\_I5 

_I\_5V\_ADC — INA180A2 output._ 
```C++
#define PDU_PIN_I5 `PA3`
```




<hr>



### define PDU\_PIN\_NTC 

_NTC\_ADC — 10k from +3V3, NTC to GND._ 
```C++
#define PDU_PIN_NTC `PA6`
```




<hr>



### define PDU\_PIN\_V12 

_12V\_READ\_ADC — 33k/10k divider_ 
```C++
#define PDU_PIN_V12 `PA0`
```




<hr>



### define PDU\_PIN\_V5 

_5V\_READ\_ADC — 10k/13k divider_ 
```C++
#define PDU_PIN_V5 `PA1`
```




<hr>



### define PDU\_SAMPLE\_US 

_rail scan period (1 kHz target)_ 
```C++
#define PDU_SAMPLE_US `1000`
```




<hr>



### define PDU\_TEMP\_CRIT\_DC 

_board temperature critical, 75.0 °C_ 
```C++
#define PDU_TEMP_CRIT_DC `750`
```




<hr>



### define PDU\_TEMP\_HYST\_DC 

_temperature-threshold hysteresis, 3.0 °C_ 
```C++
#define PDU_TEMP_HYST_DC `30`
```




<hr>



### define PDU\_TEMP\_WARN\_DC 

_board temperature warning, 60.0 °C_ 
```C++
#define PDU_TEMP_WARN_DC `600`
```




<hr>



### define PDU\_V\_HYST\_MV 

_voltage-threshold hysteresis_ 
```C++
#define PDU_V_HYST_MV `100`
```




<hr>



### define PDU\_V\_SET\_MS 

_a voltage condition must persist this long to set_ 
```C++
#define PDU_V_SET_MS `50`
```




<hr>



### define PDU\_ZERO\_GUARD\_12V\_MA 

_CAL ZERO refuses if the 12 V raw current exceeds this (load present)_ 
```C++
#define PDU_ZERO_GUARD_12V_MA `75`
```




<hr>



### define PDU\_ZERO\_GUARD\_5V\_MA 

_5 V guard — higher: the PDU's own ~30–50 mA flows through this shunt_ 
```C++
#define PDU_ZERO_GUARD_5V_MA `120`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/PduConfig.h`

