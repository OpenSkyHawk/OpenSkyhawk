# A-4E-C DCS-BIOS Address Reference

Module base address: `0x8400`. All addresses are in the range `0x8400`–`0x8554`.

## Address Format

Each `#define` in `Addresses.h` expands to `address, mask, shift` for use with `DcsBios::IntegerBuffer`:
```cpp
DcsBios::IntegerBuffer buf(A_4E_C_RPM, onRpmChange);
```
The `_A` suffix variant provides the address only (for `StringBuffer` or direct reads).
The `_AM` suffix variant provides `address, mask` (for `ChangeDispatcher`).

## Direction Key

| Symbol | Meaning |
|--------|---------|
| OUT | DCS → cockpit (gauge needle, indicator light, display digit) |
| IN | Cockpit → DCS (switch, button, knob — also echoed back as output) |
| EXT | External aircraft model (animation args) |

---

## Engine Instruments

| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `RPM` | 0x8400 | 0xFFFF | OUT | Engine RPM (0–65535 = 0–100%) |
| `RPM_DECI` | 0x8402 | 0xFFFF | OUT | Engine RPM decimal digit |
| `EGT_C` | 0x8404 | 0xFFFF | OUT | Exhaust Gas Temperature |
| `D_FUEL_FLOW` | 0x8406 | 0xFFFF | OUT | Fuel flow gauge |
| `OIL_PRESSURE` | 0x8408 | 0xFFFF | OUT | Oil pressure gauge |
| `PRESSURE_RATIO` | 0x840A | 0xFFFF | OUT | Pressure ratio gauge |
| `D_FUEL` | 0x840C | 0xFFFF | OUT | Fuel quantity gauge |

---

## Mechanical Systems Indicators

| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `D_FLAPS_IND` | 0x840E | 0xFFFF | OUT | Flap position indicator needle |
| `D_TAIL_HOOK` | 0x8410 | 0xFFFF | OUT | Hook position indicator |
| `GEAR_NOSE` | 0x8412 | 0xFFFF | OUT | Nose gear indicator |
| `GEAR_LEFT` | 0x8414 | 0xFFFF | OUT | Left gear indicator |
| `GEAR_RIGHT` | 0x8416 | 0xFFFF | OUT | Right gear indicator |
| `CANOPY_POS` | 0x84EC | 0xFFFF | OUT | Canopy position |

---

## Main Flight Instruments

### Standby Attitude Indicator (SAI)
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `ATTGYRO_STBY_HORIZ` | 0x8418 | 0xFFFF | OUT | SAI horizon bar |
| `ATTGYRO_STBY_PITCH` | 0x841A | 0xFFFF | OUT | SAI pitch |
| `ATTGYRO_STBY_ROLL` | 0x841C | 0xFFFF | OUT | SAI roll |
| `ATTGYRO_STBY_OFF` | 0x841E | 0xFFFF | OUT | SAI off flag |

### IAS / Mach
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `D_IAS_DEG` | 0x8420 | 0xFFFF | OUT | IAS needle |
| `D_IAS_MACH_DEG` | 0x8422 | 0xFFFF | OUT | Mach disc |
| `D_IAS_IDX` | 0x8424 | 0xFFFF | OUT | IAS index bug |
| `D_MACH_IDX` | 0x8426 | 0xFFFF | OUT | Mach index |

### Radar Altimeter
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `D_RADAR_ALT` | 0x8428 | 0xFFFF | OUT | Radar altimeter needle |
| `D_RADAR_IDX` | 0x842A | 0xFFFF | OUT | Radar alt indexer |
| `D_RADAR_OFF` | 0x842C | 0xFFFF | OUT | Radar alt off flag |
| `RADAR_ALT_INDEX` | 0x8510 | 0xFFFF | IN | Warning index knob |
| `RADAR_ALT_SW` | 0x850E | 0x8000 | IN | Warning button |

### Barometric Altimeter
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `D_ALT_NEEDLE` | 0x842E | 0xFFFF | OUT | Altimeter needle |
| `D_ALT_10K` | 0x8430 | 0xFFFF | OUT | 10,000s drum |
| `D_ALT_1K` | 0x8432 | 0xFFFF | OUT | 1,000s drum |
| `D_ALT_100S` | 0x8434 | 0xFFFF | OUT | 100s drum |
| `ALT_ADJ_NN00` | 0x8436 | 0xFFFF | OUT | Baro setting NNxx digits |
| `ALT_ADJ_00N0` | 0x8438 | 0xFFFF | OUT | Baro setting xxNx digit |
| `ALT_ADJ_000N` | 0x843A | 0xFFFF | OUT | Baro setting xxxN digit |
| `ALT_PRESS_KNB` | 0x8516 | 0xFFFF | IN | Baro setting knob |

### Other Flight Instruments
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `CABIN_ALT` | 0x843C | 0xFFFF | OUT | Cabin altitude gauge |
| `LIQUID_O2` | 0x843E | 0xFFFF | OUT | Liquid oxygen quantity |
| `D_OXYGEN_OFF` | 0x8440 | 0xFFFF | OUT | Oxygen off flag |
| `ACCEL_CUR` | 0x8442 | 0xFFFF | OUT | Accelerometer current G |
| `ACCEL_MAX` | 0x8444 | 0xFFFF | OUT | Accelerometer max G needle |
| `ACCEL_MIN` | 0x8446 | 0xFFFF | OUT | Accelerometer min G needle |
| `ACCEL_RESET` | 0x850E | 0x2000 | IN | Accelerometer reset button |
| `VVI` | 0x8448 | 0xFFFF | OUT | Vertical velocity indicator |
| `COMPASS_HDG` | 0x8456 | 0xFFFF | OUT | Backup compass heading |
| `AOA_G` | 0x8458 | 0xFFFF | OUT | Angle of attack gauge |
| `AOA_INDEX_DIM` | 0x851A | 0xFFFF | IN | AOA indexer dimming wheel |

### ADI (Attitude Direction Indicator)
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `ADI_PITCH` | 0x844A | 0xFFFF | OUT | ADI pitch |
| `ADI_ROLL` | 0x844C | 0xFFFF | OUT | ADI roll |
| `ADI_HDG` | 0x844E | 0xFFFF | OUT | ADI heading |
| `ADI_OFF` | 0x8450 | 0xFFFF | OUT | ADI off flag |
| `ADI_SLIP` | 0x8452 | 0xFFFF | OUT | ADI slip ball |
| `ADI_TURN` | 0x8454 | 0xFFFF | OUT | ADI turn rate |

---

## Warning & Indicator Lights

### Glareshield Warning Lights (packed at 0x845C)
| Constant | Address | Mask | Bit | Description |
|----------|---------|------|-----|-------------|
| `D_FUELBOOST_CAUTION` | 0x845C | 0x0001 | 0 | Fuel boost caution (yellow) |
| `D_CONTHYD_CAUTION` | 0x845C | 0x0002 | 1 | Control hydraulics caution (yellow) |
| `D_UTILHYD_CAUTION` | 0x845C | 0x0004 | 2 | Utility hydraulics caution (yellow) |
| `D_FUELTRANS_CAUTION` | 0x845C | 0x0008 | 3 | Fuel transfer caution (yellow) |
| `D_SPDBRK_CAUTION` | 0x845C | 0x0010 | 4 | Speed brake caution (yellow) |
| `D_SPOILER_CAUTION` | 0x845C | 0x0020 | 5 | Spoiler caution (yellow) |
| `D_RADAR_WARN` | 0x845C | 0x0040 | 6 | Radar altitude warning (red) |
| `D_OIL_LOW` | 0x845C | 0x0080 | 7 | Oil low (yellow) |
| `D_GLARE_WHEELS` | 0x845C | 0x0100 | 8 | Wheels (white) |
| `D_GLARE_LABS` | 0x845C | 0x0200 | 9 | LABS (yellow) |
| `D_GLARE_LAWS` | 0x845C | 0x0400 | 10 | LAWS (red) |
| `D_GLARE_OBST` | 0x845C | 0x0800 | 11 | OBST (yellow) |
| `D_GLARE_IFF` | 0x845C | 0x1000 | 12 | IFF (white) |
| `D_GLARE_FIRE` | 0x845C | 0x2000 | 13 | Fire (red) |
| `D_OXYGEN_LOW` | 0x845C | 0x4000 | 14 | Oxygen low (red) |
| `GEAR_LIGHT` | 0x845C | 0x8000 | 15 | Gear unsafe (red) |

### AOA / Advisory / ECM Indicator Lights (packed at 0x8470)
| Constant | Address | Mask | Bit | Description |
|----------|---------|------|-----|-------------|
| `AOA_GREEN` | 0x8470 | 0x0001 | 0 | AOA indexer green (on-speed) |
| `AOA_YELLOW` | 0x8470 | 0x0002 | 1 | AOA indexer yellow (slow) |
| `AOA_RED` | 0x8470 | 0x0004 | 2 | AOA indexer red (fast) |
| `AWRS_POWER` | 0x8470 | 0x0008 | 3 | AWRS power (yellow) |
| `APC_LIGHT` | 0x8470 | 0x0010 | 4 | APC indicator (red) |
| `D_ADVISORY_INRANGE` | 0x8470 | 0x0020 | 5 | In range (yellow) |
| `D_ADVISORY_SETRANGE` | 0x8470 | 0x0040 | 6 | Set range (yellow) |
| `D_ADVISORY_DIVE` | 0x8470 | 0x0080 | 7 | Dive (yellow) |
| `APN153_MEMORYLIGHT` | 0x8470 | 0x0100 | 8 | Doppler memory (yellow) |
| `RWR_LIGHT` | 0x8470 | 0x0200 | 9 | RWR glareshield light |
| `ECM_TEST_LIGHT` | 0x8470 | 0x0400 | 10 | ECM test (white) |
| `ECM_GO_LIGHT` | 0x8470 | 0x0800 | 11 | ECM GO (green) |
| `ECM_NOGO_LIGHT` | 0x8470 | 0x1000 | 12 | ECM NO GO (red) |
| `ECM_SAM_LIGHT` | 0x8470 | 0x2000 | 13 | ECM SAM (red) |
| `ECM_RPT_LIGHT` | 0x8470 | 0x4000 | 14 | ECM RPT (green) |
| `ECM_STBY_LIGHT` | 0x8470 | 0x8000 | 15 | ECM STBY (yellow) |

---

## Gunsight
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `D_GUNSIGHT_REFLECTOR` | 0x845A | 0xFFFF | OUT | Reflector elevation |
| `GUNSIGHT_BRIGHT` | 0x851C | 0xFFFF | IN | Light control knob |
| `GUNSIGHT_DAY_NIGHT` | 0x8512 | 0x0010 | IN | Day/night switch |
| `GUNSIGHT_KNB` | 0x851E | 0xFFFF | IN | Elevation control knob |

---

## BDHI (Bearing Distance Heading Indicator)
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `BDHI_HDG` | 0x845E | 0xFFFF | OUT | Heading card |
| `BDHI_NEEDLE1` | 0x8460 | 0xFFFF | OUT | Needle 1 |
| `BDHI_NEEDLE2` | 0x8462 | 0xFFFF | OUT | Needle 2 |
| `BDHI_DME_FLAG` | 0x8464 | 0xFFFF | OUT | DME flag |
| `BDHI_DME_X00` | 0x8466 | 0xFFFF | OUT | DME Xnn digit |
| `BDHI_DME_0X0` | 0x8468 | 0xFFFF | OUT | DME nXn digit |
| `BDHI_DME_00X` | 0x846A | 0xFFFF | OUT | DME nnX digit |
| `BDHI_ILS_GS` | 0x846C | 0xFFFF | OUT | ILS glideslope needle |
| `BDHI_ILS_LOC` | 0x846E | 0xFFFF | OUT | ILS localizer needle |
| `BDHI_MODE` | 0x8502 | 0x0300 | IN | BDHI mode switch |

---

## Radar (APG-53A)
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `APG53A_LEFTRANGE` | 0x8472 | 0xFFFF | OUT | Profile range scale |
| `APG53A_BOTTOMRANGE` | 0x8474 | 0xFFFF | OUT | Plan range scale |
| `APG53A_GLOW` | 0x84DE | 0xFFFF | OUT | Scope glow light (green) |
| `RADAR_MODE` | 0x84EA | 0x000E | IN | Mode selector (5-pos) |
| `RADAR_AOACOMP` | 0x84EA | 0x0010 | IN | AoA compensation toggle |
| `RADAR_ANGLE` | 0x84EE | 0xFFFF | IN | Antenna tilt pot |
| `RADAR_VOL` | 0x84F0 | 0xFFFF | IN | Obstacle tone volume |
| `RADAR_STORAGE` | 0x84F2 | 0xFFFF | IN | Scope storage knob |
| `RADAR_BRILLIANCE` | 0x84F4 | 0xFFFF | IN | Scope brilliance knob |
| `RADAR_DETAIL` | 0x84F6 | 0xFFFF | IN | Scope detail knob |
| `RADAR_GAIN` | 0x84F8 | 0xFFFF | IN | Scope gain knob |
| `RADAR_RETICLE` | 0x84FA | 0xFFFF | IN | Scope reticle knob |
| `RADAR_FILTER` | 0x84EA | 0x0020 | IN | Filter plate toggle |
| `RADAR_PROFILE` | 0x8500 | 0x8000 | IN | Plan/profile toggle |
| `RADAR_RANGE` | 0x8502 | 0x0080 | IN | Long/short range toggle |

---

## AFCS Panel
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `AFCS_HDG_100S` | 0x8476 | 0xFFFF | OUT | Heading presel 100s drum |
| `AFCS_HDG_10S` | 0x8478 | 0xFFFF | OUT | Heading presel 10s drum |
| `AFCS_HDG_1S` | 0x847A | 0xFFFF | OUT | Heading presel 1s drum |
| `AFCS_ROLL` | 0x854E | 0xFFFF | OUT | AFCS roll gauge |
| `AFCS_YAW` | 0x8550 | 0xFFFF | OUT | AFCS yaw gauge |
| `AFCS_PITCH` | 0x8552 | 0xFFFF | OUT | AFCS pitch gauge |
| `AFCS_STBY` | 0x8502 | 0x2000 | IN | Standby toggle |
| `AFCS_ENGAGE` | 0x8502 | 0x4000 | IN | Engage toggle |
| `AFCS_HDG_SEL` | 0x8502 | 0x8000 | IN | Preselect heading toggle |
| `AFCS_ALT` | 0x8508 | 0x0001 | IN | Altitude hold toggle |
| `AFCS_STAB_AUG` | 0x8508 | 0x0002 | IN | Stability aug toggle |
| `AFCS_AIL_TRIM` | 0x8508 | 0x0004 | IN | Aileron trim toggle |
| `AFCS_HDG_SET` | 0x850A | 0xFFFF | IN | Heading selector knob |
| `AFCS_1N2` | 0x854C | 0x0018 | IN | 1-N-2 guard (3-pos) |
| `AFCS_1N2_COVER` | 0x854C | 0x0004 | IN | 1-N-2 guard cover |

---

## Approach Power Compensator (APC)
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `APC_LIGHT` | 0x8470 | 0x0010 | OUT | APC indicator (red) — see lights table |
| `APC_ENABLE` | 0x8508 | 0x0018 | IN | APC enable/stby/off (3-pos) |
| `APS_COLD_STD_HOT` | 0x8508 | 0x0060 | IN | Cold/std/hot (3-pos) |

---

## Armament Panel
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `ARM_MASTER` | 0x8500 | 0x4000 | IN | Master arm toggle |
| `ARM_GUN` | 0x84EA | 0x8000 | IN | Guns switch |
| `ARM_BOMB` | 0x8500 | 0x0018 | IN | Bomb arm (3-pos) |
| `ARM_EMERG_SEL` | 0x8500 | 0x0007 | IN | Emergency release selector (7-pos) |
| `ARM_FUNC_SEL` | 0x8500 | 0x1C00 | IN | Function selector (7-pos) |
| `ARM_STATION1` | 0x8500 | 0x0020 | IN | Station 1 select |
| `ARM_STATION2` | 0x8500 | 0x0040 | IN | Station 2 select |
| `ARM_STATION3` | 0x8500 | 0x0080 | IN | Station 3 select |
| `ARM_STATION4` | 0x8500 | 0x0100 | IN | Station 4 select |
| `ARM_STATION5` | 0x8500 | 0x0200 | IN | Station 5 select |
| `AWRS_QUANT` | 0x8502 | 0x000F | IN | Quantity selector (12-pos) |
| `AWRS_DROP_INT` | 0x8504 | 0xFFFF | IN | Drop interval pot |
| `AWRS_MULTI` | 0x8500 | 0x2000 | IN | Multiplier toggle |
| `AWRS_MODE` | 0x8502 | 0x0070 | IN | Mode selector (6-pos) |
| `MISSILE_VOL` | 0x8506 | 0xFFFF | IN | Missile volume knob |
| `SHRIKE_SEL_KNB` | 0x8502 | 0x1C00 | IN | Shrike selector (5-pos) |
| `GUNPOD_CLEAR` | 0x84EA | 0x00C0 | IN | Gunpod charge/off/clear |
| `GUNPOD_L` | 0x84EA | 0x0100 | IN | Gunpod LH station |
| `GUNPOD_C` | 0x84EA | 0x0200 | IN | Gunpod CTR station |
| `GUNPOD_R` | 0x84EA | 0x0400 | IN | Gunpod RH station |

---

## Mechanical Systems
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `GEAR_HANDLE` | 0x8508 | 0x0080 | IN | Landing gear handle |
| `HOOK_HANDLE` | 0x8508 | 0x0100 | IN | Arresting hook handle |
| `FLAPS` | 0x8508 | 0xC000 | IN | Flaps lever (3-pos) |
| `SPEEDBRAKE` | 0x8508 | 0x0400 | IN | Speed brake toggle |
| `SPEEDBRAKE_EMERG` | 0x8508 | 0x1800 | IN | Speed brake emergency (3-pos) |
| `SPOILER_ARM` | 0x8508 | 0x0200 | IN | Spoiler arm toggle |
| `CANOPY_SW` | 0x8508 | 0x2000 | IN | Canopy toggle |
| `RUDDER_TRIM` | 0x850C | 0xFFFF | IN | Rudder trim pot |
| `THROTTLE_CLICK` | 0x850E | 0x0003 | IN | Throttle cutoff/start/idle (3-pos) |
| `STARTER_BTN` | 0x850E | 0x0004 | IN | Starter button |
| `JATO_ARM` | 0x850E | 0x0008 | IN | JATO arm toggle |
| `JATO_JETT_SAFE` | 0x850E | 0x0010 | IN | JATO jettison/safe |
| `SEAT_ADJ` | 0x854C | 0x3000 | IN | Seat adjustment (3-pos) |

---

## Fuel Systems
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `DROP_PRESS_REFUEL` | 0x850E | 0x0060 | IN | Drop tanks press / refuel (3-pos) |
| `EMERG_TRANS_FUEL_DUMP` | 0x850E | 0x0180 | IN | Emergency transfer / wing dump (3-pos) |
| `FUEL_CONTROL` | 0x850E | 0x0200 | IN | Fuel control toggle |
| `MAN_FUEL_OFF_LV` | 0x850E | 0x0400 | IN | Manual fuel shutoff lever |
| `MAN_FUEL_OFF_CATCH` | 0x850E | 0x0800 | IN | Manual fuel shutoff catch |
| `FUEL_TRANS` | 0x8554 | 0x0002 | IN | Fuel transfer bypass/normal |
| `FUEL_EXT_BTN` | 0x8512 | 0x0002 | IN | Show EXT fuel button |

---

## Avionics / Misc
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `MASTER_TEST` | 0x8512 | 0x0004 | IN | Master test button |
| `IAS_INDEX_BTN` | 0x8512 | 0x0008 | IN | IAS index button |
| `IAS_INDEX_KNB` | 0x8518 | 0xFFFF | IN | IAS index knob |
| `STBY_ATT_INDEX_BTN` | 0x8512 | 0x0001 | IN | SAI horizon button |
| `STBY_ATT_INDEX_KNB` | 0x8514 | 0xFFFF | IN | SAI horizon knob |
| `OXY_SW` | 0x850E | 0x1000 | IN | Oxygen switch |
| `RAIN_REMOVE` | 0x8554 | 0x0004 | IN | Rain removal toggle |

---

## ECM Panel
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `ECM_REC_LIGHT` | 0x84EA | 0x0001 | OUT | REC light (red) |
| `ECM_AUDIO` | 0x853E | 0x0200 | IN | Audio APR/25-APR/27 toggle |
| `ECM_APR25_PW` | 0x853E | 0x0400 | IN | APR/25 on/off |
| `ECM_APR27_PW` | 0x853E | 0x0800 | IN | APR/27 on/off |
| `ECM_APR27_TEST` | 0x853E | 0x1000 | IN | APR/27 test button |
| `ECM_APR27_LIGHT` | 0x853E | 0x2000 | IN | APR/27 light button |
| `ECM_PRF_VOL` | 0x8540 | 0xFFFF | IN | PRF volume (inner knob) |
| `ECM_MSL_VOL` | 0x8542 | 0xFFFF | IN | MSL volume (outer knob) |
| `ECM_SEL` | 0x853E | 0xC000 | IN | AN/APR-25 selector (4-pos) |

---

## UHF Radio (AN/ARC-51)

### Display outputs
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `ARC51_FREQ_XX000` | 0x84CE | 0xFFFF | OUT | Frequency XXnnn display |
| `ARC51_FREQ_00X00` | 0x84D0 | 0xFFFF | OUT | Frequency nnXnn display |
| `ARC51_FREQ_000XX` | 0x84D2 | 0xFFFF | OUT | Frequency nnnXX display |
| `ARC51_FREQ_PRESET` | 0x84D4 | 0xFFFF | OUT | Preset channel display |

### Controls
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `ARC51_FREQ_PRE` | 0x853A | 0x001F | IN | Preset channel selector (20-pos) |
| `ARC51_FREQ_10MHZ` | 0x853A | 0x03E0 | IN | Freq 10 MHz dial (18-pos) |
| `ARC51_FREQ_1MHZ` | 0x853A | 0x3C00 | IN | Freq 1 MHz dial (10-pos) |
| `ARC51_FREQ_50KHZ` | 0x853E | 0x001F | IN | Freq 50 kHz dial (20-pos) |
| `ARC51_MODE` | 0x853A | 0xC000 | IN | Mode selector (4-pos) |
| `ARC51_XMIT_MODE` | 0x8532 | 0x3000 | IN | Frequency mode (3-pos) |
| `ARC51_SQUELCH` | 0x8532 | 0x4000 | IN | Squelch disable toggle |
| `ARC51_VOL` | 0x853C | 0xFFFF | IN | Volume knob |

---

## TACAN
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `TACAN_MODE` | 0x8512 | 0x0060 | IN | Mode selector (4-pos) |
| `TACAN_CHAN_MAJ` | 0x8512 | 0x0780 | IN | Channel major (13-pos) |
| `TACAN_CHAN_MIN` | 0x8512 | 0x7800 | IN | Channel minor (10-pos) |
| `TACAN_VOL` | 0x8520 | 0xFFFF | IN | Volume knob |
| `TACAN_ANT_CONT` | 0x854C | 0xC000 | IN | Antenna control (3-pos) |

---

## Doppler Navigation (APN-153 / ASN-41)

### Displays
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `APN153_DRIFT_GAUGE` | 0x847C | 0xFFFF | OUT | Drift gauge |
| `APN153_SPEED_X00` | 0x847E | 0xFFFF | OUT | Speed Xnn |
| `APN153_SPEED_0X0` | 0x8480 | 0xFFFF | OUT | Speed nXn |
| `APN153_SPEED_00X` | 0x8482 | 0xFFFF | OUT | Speed nnX |
| `NAV_CURPOS_LAT_X0000`–`_0000X` | 0x8484–0x848C | 0xFFFF | OUT | Current lat (5 digits) |
| `NAV_CURPOS_LON_X00000`–`_00000X` | 0x848E–0x8498 | 0xFFFF | OUT | Current lon (6 digits) |
| `NAV_DEST_LAT_X0000`–`_0000X` | 0x849A–0x84A2 | 0xFFFF | OUT | Dest lat (5 digits) |
| `NAV_DEST_LON_X00000`–`_00000X` | 0x84A4–0x84AE | 0xFFFF | OUT | Dest lon (6 digits) |
| `ASN41_WINDSPEED_X00`–`_00X` | 0x84B0–0x84B4 | 0xFFFF | OUT | Wind speed (3 digits) |
| `ASN41_WINDDIR_X00`–`_00X` | 0x84B6–0x84BA | 0xFFFF | OUT | Wind direction (3 digits) |
| `ASN41_MAGVAR_X0000`–`_0000X` | 0x84BC–0x84C4 | 0xFFFF | OUT | Magnetic variation (5 digits) |

### Controls
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `DOPPLER_SEL` | 0x8522 | 0x0007 | IN | APN-153 mode (5-pos) |
| `DOPPLER_MEM_TEST` | 0x8512 | 0x8000 | IN | Memory light test button |
| `NAV_SEL` | 0x8522 | 0x0038 | IN | ASN-41 function selector (5-pos) |
| `NAV_DEAD` | 0x8554 | 0x0001 | IN | Dead reckoning/doppler toggle |
| `PPOS_LAT_KNB` | 0x8524 | 0xFFFF | IN | Present position lat knob |
| `PPOS_LAT_BTN` | 0x8522 | 0x0040 | IN | Present position lat button |
| `PPOS_LON_KNB` | 0x8526 | 0xFFFF | IN | Present position lon knob |
| `PPOS_LON_BTN` | 0x8522 | 0x0080 | IN | Present position lon button |
| `DEST_LAT_KNB` | 0x8528 | 0xFFFF | IN | Destination lat knob |
| `DEST_LAT_BTN` | 0x8522 | 0x0100 | IN | Destination lat button |
| `DEST_LON_KNB` | 0x852A | 0xFFFF | IN | Destination lon knob |
| `DEST_LON_BTN` | 0x8522 | 0x0200 | IN | Destination lon button |
| `ASN41_MAGVAR_KNB` | 0x852C | 0xFFFF | IN | Magnetic variation knob |
| `ASN41_MAGVAR_BTN` | 0x8522 | 0x0400 | IN | Magnetic variation button |
| `ASN41_WINDSPEED_KNB` | 0x852E | 0xFFFF | IN | Wind speed knob |
| `ASN41_WINDSPEED_BTN` | 0x8522 | 0x0800 | IN | Wind speed button |
| `ASN41_WINDDIR_KNB` | 0x8530 | 0xFFFF | IN | Wind direction knob |
| `ASN41_WINDDIR_BTN` | 0x8522 | 0x1000 | IN | Wind direction button |
| `ASN41_LAT_SLEW` | 0x8544 | 0x0060 | IN | Dest lat slew (3-pos) |
| `ASN41_LON_SLEW` | 0x8544 | 0x0180 | IN | Dest lon slew (3-pos) |

---

## Countermeasures
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `CM_BANK1_X0` | 0x84C6 | 0xFFFF | OUT | Bank 1 count 10s digit |
| `CM_BANK1_0X` | 0x84C8 | 0xFFFF | OUT | Bank 1 count 1s digit |
| `CM_BANK2_X0` | 0x84CA | 0xFFFF | OUT | Bank 2 count 10s digit |
| `CM_BANK2_0X` | 0x84CC | 0xFFFF | OUT | Bank 2 count 1s digit |
| `CM_BANK` | 0x84EA | 0x1800 | IN | Bank select (3-pos) |
| `CM_AUTO` | 0x84EA | 0x2000 | IN | Auto button |
| `CM_PWR` | 0x84EA | 0x4000 | IN | Power toggle |
| `CM_ADJ1` | 0x84FC | 0xFFFF | IN | Bank 1 adjust knob |
| `CM_ADJ2` | 0x84FE | 0xFFFF | IN | Bank 2 adjust knob |

---

## Cockpit Lighting

### Outputs (ambient levels exported by DCS)
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `LIGHTS_FLOOD_WHITE` | 0x84D6 | 0xFFFF | OUT | White flood level |
| `LIGHTS_FLOOD_RED` | 0x84D8 | 0xFFFF | OUT | Red flood level |
| `LIGHTS_INSTRUMENTS` | 0x84DA | 0xFFFF | OUT | Instrument lighting level |
| `LIGHTS_CONSOLE` | 0x84DC | 0xFFFF | OUT | Console lighting level |
| `APG53A_GLOW` | 0x84DE | 0xFFFF | OUT | Radar scope glow |

### Interior light controls
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `LIGHT_INT_INSTR` | 0x8534 | 0xFFFF | IN | Instrument lighting pot |
| `LIGHT_INT_CONSOLE` | 0x8536 | 0xFFFF | IN | Console lighting pot |
| `LIGHT_INT_FLOOD_WHT` | 0x8538 | 0xFFFF | IN | White floodlight pot |
| `LIGHT_INT_BRIGHT` | 0x8532 | 0x0C00 | IN | Console intensity (3-pos) |

### Exterior light controls
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `LIGHT_EXT_MASTER` | 0x8522 | 0x6000 | IN | Master lighting (3-pos) |
| `LIGHT_EXT_TAXI` | 0x8522 | 0x8000 | IN | Taxi light |
| `LIGHT_EXT_PROBE` | 0x8532 | 0x0003 | IN | Probe light (3-pos) |
| `LIGHT_EXT_ANTICOLL` | 0x8532 | 0x0004 | IN | Anti-collision toggle |
| `LIGHT_EXT_FUSELAGE` | 0x8532 | 0x0018 | IN | Fuselage lights (3-pos) |
| `LIGHT_EXT_NAV` | 0x8532 | 0x00C0 | IN | Navigation lights (3-pos) |
| `LIGHT_EXT_TAIL` | 0x8532 | 0x0300 | IN | Tail light (3-pos) |
| `LIGHT_EXT_FLASH_MODE` | 0x8532 | 0x0020 | IN | Flash/steady mode toggle |

---

## Clock
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `CURRTIME_HOURS` | 0x84E0 | 0xFFFF | OUT | Current hours |
| `CURRTIME_MINS` | 0x84E2 | 0xFFFF | OUT | Current minutes |
| `CURRTIME_SECS` | 0x84E4 | 0xFFFF | OUT | Current seconds |
| `STOPWATCH_MINS` | 0x84E6 | 0xFFFF | OUT | Stopwatch minutes |
| `STOPWATCH_SECS` | 0x84E8 | 0xFFFF | OUT | Stopwatch seconds |
| `STOPWATCH` | 0x850E | 0x4000 | IN | Stopwatch start/stop button |

---

## Air Conditioning
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `CABIN_PRESS` | 0x8544 | 0x0001 | IN | Cabin pressure toggle |
| `WINDSHLD_DEFROST` | 0x8544 | 0x0006 | IN | Windshield defrost (3-pos) |
| `CABIN_TEMP` | 0x8546 | 0xFFFF | IN | Cabin temp knob |

---

## T-Handles
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `EMERG_GEAR_REL` | 0x8532 | 0x8000 | IN | Emergency gear release |
| `EMERG_BOMB_REL` | 0x853E | 0x0020 | IN | Emergency bomb release |
| `EMERG_GEN_DEPLOY` | 0x853E | 0x0040 | IN | Emergency generator deploy |
| `EMERG_GEN_BYPASS` | 0x853E | 0x0080 | IN | Emergency generator bypass |
| `MAN_FLIGHT_CONTROL` | 0x853E | 0x0100 | IN | Manual flight control override |

---

## Ejection Seat
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `HARNESS_REEL_CONTR` | 0x8544 | 0x0008 | IN | Shoulder harness inertia reel |
| `SEC_EJECT_HANDLE` | 0x8544 | 0x0010 | IN | Secondary ejection handle |

---

## MCL (Master Caution Light panel)
| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `MCL_CHAN_SEL` | 0x854C | 0x03E0 | IN | Channel selector (20-pos) |
| `MCL_PWR` | 0x854C | 0x0C00 | IN | Power (3-pos) |

---

## External Aircraft Model
These are animation arguments exported for visual reference only — not used for physical cockpit outputs.

| Constant | Address | Mask | Dir | Description |
|----------|---------|------|-----|-------------|
| `EXT_SPEED_BRAKES` | 0x8548 | 0xFFFF | EXT | Speed brake position |
| `EXT_HOOK` | 0x854A | 0xFFFF | EXT | Hook position |
| `EXT_WOW_NOSE` | 0x8544 | 0x8000 | EXT | Weight on wheels — nose |
| `EXT_WOW_RIGHT` | 0x854C | 0x0001 | EXT | Weight on wheels — right |
| `EXT_WOW_LEFT` | 0x854C | 0x0002 | EXT | Weight on wheels — left |
| `EXT_POSITION_LIGHT_LEFT` | 0x8544 | 0x0200 | EXT | Left position light (red) |
| `EXT_POSITION_LIGHT_RIGHT` | 0x8544 | 0x0400 | EXT | Right position light (green) |
| `EXT_TAIL_LIGHT` | 0x8544 | 0x0800 | EXT | Tail light (white) |
| `EXT_STROBE_TOP` | 0x8544 | 0x1000 | EXT | Top strobe (red) |
| `EXT_STROBE_BOTTOM` | 0x8544 | 0x2000 | EXT | Bottom strobe (red) |
| `EXT_TAXI_LIGHT` | 0x8544 | 0x4000 | EXT | Taxi light (white) |

---

## Address Map Summary

All A-4E-C addresses fall in `0x8400`–`0x8554`. Key register boundaries:

| Range | Contents |
|-------|---------|
| 0x8400–0x841E | Engine + gear + SAI gauges |
| 0x8420–0x845A | Flight instruments (IAS, radar alt, altimeter, cabin, O2, accel, VVI, ADI, compass, AoA, gunsight) |
| 0x845C–0x845C | All glareshield warning lights (packed) |
| 0x845E–0x846E | BDHI |
| 0x8470–0x8470 | AOA indexer + advisory + ECM + Doppler lights (packed) |
| 0x8472–0x8474 | Radar scope ranges |
| 0x8476–0x847A | AFCS heading drums |
| 0x847C–0x84AE | Doppler nav displays |
| 0x84B0–0x84C4 | ASN-41 wind/magvar displays |
| 0x84C6–0x84CC | CM bank counters |
| 0x84CE–0x84D4 | UHF freq displays |
| 0x84D6–0x84DE | Cockpit light levels + radar glow |
| 0x84E0–0x84E8 | Clock |
| 0x84EA–0x84FA | Radar/ECM/gunpod controls + CM controls |
| 0x84EC | Canopy position |
| 0x84EE–0x84FA | Radar scope knobs |
| 0x84FC–0x84FE | CM adj knobs |
| 0x8500–0x8508 | Armament + AFCS + mechanical switch states |
| 0x850A–0x850C | AFCS heading set + rudder trim |
| 0x850E | Misc controls (accel reset, starter, fuel, radar alt, stopwatch...) |
| 0x8510–0x851A | Radar alt index, TACAN, gunsight, IAS, AOA knobs |
| 0x851C–0x851E | Gunsight bright + elevation |
| 0x8520–0x8530 | TACAN vol + Doppler buttons + ASN-41 knobs |
| 0x8532–0x853E | Ext lights + ECM + UHF |
| 0x8540–0x8542 | ECM volumes |
| 0x8544–0x8546 | Air conditioning + ext lights bits |
| 0x8548–0x854A | External model |
| 0x854C–0x8554 | AFCS 1N2, MCL, WoW, TACAN ant, seat, misc |
