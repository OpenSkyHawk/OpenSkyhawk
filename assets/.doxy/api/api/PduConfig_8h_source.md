

# File PduConfig.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**PduConfig.h**](PduConfig_8h.md)

[Go to the documentation of this file](PduConfig_8h.md)


```C++

#pragma once
#include <stdint.h>

// ── Pin map (Rev 0.2.0 netlist) ──────────────────────────────────────────────────
#ifndef PDU_PIN_V12
#define PDU_PIN_V12  PA0   
#endif
#ifndef PDU_PIN_V5
#define PDU_PIN_V5   PA1   
#endif
#ifndef PDU_PIN_I12
#define PDU_PIN_I12  PA2   
#endif
#ifndef PDU_PIN_I5
#define PDU_PIN_I5   PA3   
#endif
#ifndef PDU_PIN_NTC
#define PDU_PIN_NTC  PA6   
#endif

namespace OpenSkyhawk {
namespace Pdu {

// ── Analog front end ─────────────────────────────────────────────────────────────
static constexpr uint32_t ADC_FULL      = 1UL << 16;  
static constexpr uint16_t VDDA_NOMINAL_MV = 3300;     
static constexpr uint16_t VREFINT_TYP_MV  = 1200;     

static constexpr uint32_t V12_DIV_NUM = 43, V12_DIV_DEN = 10;
static constexpr uint32_t V5_DIV_NUM  = 23, V5_DIV_DEN  = 13;

static constexpr uint32_t I_MA_PER_MV = 2;

static constexpr float    NTC_R0_OHM   = 10000.0f;
static constexpr float    NTC_BETA_K   = 3450.0f;
static constexpr float    NTC_T0_K     = 298.15f;
static constexpr float    NTC_BIAS_OHM = 10000.0f;

static constexpr uint16_t GAIN_UNITY = 10000;  

} // namespace Pdu
} // namespace OpenSkyhawk

// ── Thresholds (override with -D) ─────────────────────────────────────────────────
#ifndef PDU_12V_UV_MV
#define PDU_12V_UV_MV      11400   
#endif
#ifndef PDU_12V_OV_MV
#define PDU_12V_OV_MV      13200   
#endif
#ifndef PDU_5V_UV_MV
#define PDU_5V_UV_MV        4750   
#endif
#ifndef PDU_5V_OV_MV
#define PDU_5V_OV_MV        5500   
#endif
#ifndef PDU_12V_OC_MA
#define PDU_12V_OC_MA       4500   
#endif
#ifndef PDU_5V_OC_MA
#define PDU_5V_OC_MA        1800   
#endif
#ifndef PDU_FUSE_OPEN_MV
#define PDU_FUSE_OPEN_MV    1000   
#endif
#ifndef PDU_V_HYST_MV
#define PDU_V_HYST_MV        100   
#endif
#ifndef PDU_V_SET_MS
#define PDU_V_SET_MS          50   
#endif
#ifndef PDU_OC_SET_MS
#define PDU_OC_SET_MS        100   
#endif
#ifndef PDU_CLEAR_MS
#define PDU_CLEAR_MS         500   
#endif
#ifndef PDU_TEMP_WARN_DC
#define PDU_TEMP_WARN_DC     600   
#endif
#ifndef PDU_TEMP_CRIT_DC
#define PDU_TEMP_CRIT_DC     750   
#endif
#ifndef PDU_TEMP_HYST_DC
#define PDU_TEMP_HYST_DC      30   
#endif
#ifndef PDU_BOOT_BLANK_MS
#define PDU_BOOT_BLANK_MS    500   
#endif
#ifndef PDU_SAMPLE_US
#define PDU_SAMPLE_US       1000   
#endif
#ifndef PDU_ZERO_GUARD_12V_MA
#define PDU_ZERO_GUARD_12V_MA 75   
#endif
#ifndef PDU_ZERO_GUARD_5V_MA
#define PDU_ZERO_GUARD_5V_MA 120   
#endif
#ifndef PDU_CAL_PAGE_ADDR
#define PDU_CAL_PAGE_ADDR 0x0800FC00UL   
#endif
```


