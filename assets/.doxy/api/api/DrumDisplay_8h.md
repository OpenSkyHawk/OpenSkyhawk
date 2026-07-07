

# File DrumDisplay.h



[**FileList**](files.md) **>** [**DrumDisplay**](dir_b8ac8c4bf654b399fff34fe5003d7d6c.md) **>** [**DrumDisplay.h**](DrumDisplay_8h.md)

[Go to the source code of this file](DrumDisplay_8h_source.md)

_Rolling mechanical-drum OLED readout output for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) __[_**PanelGroup**_](namespacePanelGroup.md) _nodes._[More...](#detailed-description)

* `#include <PanelGroup.h>`
* `#include <NodeStatus.h>`
* `#include <U8g2lib.h>`
* `#include <Helpers/I2cMux/I2cMux.h>`
* `#include <Helpers/I2cHealth/I2cHealth.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**DrumDisplay**](classOpenSkyhawk_1_1DrumDisplay.md) <br>_Rolling-drum OLED readout. One instance == one OLED panel._  |
| struct | [**DrumFlag**](structOpenSkyhawk_1_1DrumFlag.md) <br>_Optional 2-state (or N-state) flag tape — hemisphere N/S · E/W, or a mode letter._  |
| struct | [**DrumGlyph**](structOpenSkyhawk_1_1DrumGlyph.md) <br>_A fixed (non-rolling) glyph painted between digit columns — '.', ' ', ':' etc._  |
| struct | [**DrumReadout**](structOpenSkyhawk_1_1DrumReadout.md) <br>_Complete description of one rolling readout: its sources, geometry, glyphs, flag._  |
| struct | [**DrumSource**](structOpenSkyhawk_1_1DrumSource.md) <br>_One DCS-BIOS digit source feeding a_ [_**DrumReadout**_](structOpenSkyhawk_1_1DrumReadout.md) _._ |


















































## Detailed Description


One DrumDisplay instance drives ONE OLED panel through a caller-owned U8G2 object. It renders a row of digit "tapes" that ease toward their target value with a mechanical-drum cascade (units rolls continuously; higher places dwell and roll only on carry), optionally followed by a 2-state hemisphere/mode flag tape. Multi-digit DCS-BIOS readouts (APN-153 SPEED, NAV LAT/LON, MAGVAR, ARC-51 frequency, BDHI range) are reconstructed from their per-digit DCS-BIOS output addresses via a DrumReadout descriptor.


Renderer ported from the bench-verified SH1106 prototype (Tests/DrumDisplay roll\_reference): per-cell setClipWindow + drawTape/drawFlag + per-place ease. Adds snap-then-settle, DCS-BIOS address→digit decode, and descriptor-driven geometry auto-fit.


Ownership: the sketch constructs the concrete U8G2 type (it knows the panel — SH1106 vs SSD1306, size, rotation), calls Wire.begin() and oled.begin(), and passes the U8G2 by reference. DrumDisplay calls only base-class U8G2 methods. For N panels on one TCA9548A, construct N DrumDisplay instances sharing one I2cMux, each with its own channel; the class re-selects its channel before every buffer send.




**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/DrumDisplay/DrumDisplay.h`

