

# File ShiftBus.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Helpers**](dir_9e93d9a1721bcf27b2030ff612e0fc11.md) **>** [**ShiftBus**](dir_5de82edf055e68e6d2d76fc20b67149e.md) **>** [**ShiftBus.h**](ShiftBus_8h.md)

[Go to the source code of this file](ShiftBus_8h_source.md)

_Shared SPI shift-register bus — 74HC165 input chain + 74HC595 output chain._ [More...](#detailed-description)

* `#include <Arduino.h>`
* `#include <SPI.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**ShiftBus**](classOpenSkyhawk_1_1ShiftBus.md) <br>_One shared SPI shift-register bus ('165 inputs + '595 outputs)._  |






## Public Attributes

| Type | Name |
| ---: | :--- |
|  [**OpenSkyhawk::ShiftBus**](classOpenSkyhawk_1_1ShiftBus.md) | [**ShiftBus1**](#variable-shiftbus1)  <br>_Pre-defined bus on the standard pins — SPI1-remap SCK=PB3 / MISO=PB4 / MOSI=PB5, LOAD=PB8, LATCH=PB9 (one contiguous header run with I2C1: PB9..PB3)._  |












































## Detailed Description


One ShiftBus instance owns one SPI bus carrying both chains: '165 (parallel-in/serial-out) on MISO and '595 (serial-in/parallel-out) on MOSI, sharing SCK. A '165 read clocks the shared SCK and scrambles the '595 shift stage (latched outputs are unaffected), so every transaction is one full-duplex transfer(): pulse LOAD to capture the '165 inputs, shift the complete output frame while receiving the complete input frame, pulse LATCH to publish the '595 outputs. There is no partial read and no partial write.


Sketches normally use the pre-defined global instance `ShiftBus1` (the TwoWire/Wire pattern) and perform no setup at all — declaring an SR-backed [**PinRef**](classPinRef.md) is the only opt-in. [**PinRef::configureAsInput()**](classPinRef.md#function-configureasinput)/configureAsOutput() notify the bus (direction, chain length); [**PanelGroup::setup()**](namespacePanelGroup.md#function-setup) then begin()s every active bus. A bus with no SR pins stays dormant: SPI.begin() is never called.


TechSpec: Firmware/ScratchPad/TechSpec/PanelGroup/Helpers/ShiftBus.md (issues #197 / #133).




**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    
## Public Attributes Documentation




### variable ShiftBus1 

_Pre-defined bus on the standard pins — SPI1-remap SCK=PB3 / MISO=PB4 / MOSI=PB5, LOAD=PB8, LATCH=PB9 (one contiguous header run with I2C1: PB9..PB3)._ 
```C++
OpenSkyhawk::ShiftBus ShiftBus1;
```



Override per board with -DSHIFTBUS\_SCK=... / \_MISO / \_MOSI / \_LOAD / \_LATCH. Dormant (zero cost) unless a sketch declares a [**PinRef**](classPinRef.md) on it. 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Helpers/ShiftBus/ShiftBus.h`

