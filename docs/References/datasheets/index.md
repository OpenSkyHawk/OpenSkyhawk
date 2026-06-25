# Component Datasheets

Vendor datasheets for parts used across OpenSkyhawk boards. Filed here for reference; the
authoritative part selection lives in each board's schematic + the controller mirrors under
`docs/_source/controllers/`.

| Datasheet | Part | Used by |
|---|---|---|
| [MCP23017 (Microchip DS20001952C)](MCP23017_Microchip_DS20001952C.pdf) | MCP23017-E/SS 16-bit I²C I/O expander (SSOP-28) | every PanelGroup breakout board |
| [DRV8833 (TI)](DRV8833_TI_motor-driver.pdf) | DRV8833 dual H-bridge (HTSSOP `PWP` / TSSOP `PW`) | NeedleGauge stepper drive (e.g. APN-153 DRIFT) |
| [Switec X27 (Juken Swiss)](Switec_X27_stepper_JukenSwiss.pdf) | X27 air-core gauge stepper (2-coil bipolar, 1/3°/step, 5 V/20 mA) | NeedleGauge needles (X27.589 = APN-153 DRIFT) |
| [SSD1306 0.91″ 128×32 I²C OLED](SSD1306_OLED_0.91in_128x32_I2C.pdf) | 0.91″ SSD1306 I²C OLED module | DrumDisplay readouts (APN-153 GND SPEED) |
| [ALPHA SR26 rotary switch](ALPHA_SR26_rotary-switch.pdf) | SR26-series rotary, 30° changeover (SR2611F solder-lug / SR2612F PCB) | SwitchMultiPos selectors (APN-153 DOPPLER_SEL) |
| [R16-503 illuminated pushbutton](R16-503_illuminated_pushbutton.pdf) | 16 mm illuminated pushbutton, 3 VDC LED (no-lock `/BD` momentary · self-lock `/AD`) | Switch2Pos + LED indicators (APN-153 MEM) |
