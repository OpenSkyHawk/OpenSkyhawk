

# File PinRef.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**PinRef.h**](PinRef_8h.md)

[Go to the source code of this file](PinRef_8h_source.md)

_Hardware pin abstraction for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _panel controls._[More...](#detailed-description)

* `#include <Arduino.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**PinRef**](classPinRef.md) <br>_Hardware pin abstraction used by all_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _input and output classes._ |






## Public Attributes

| Type | Name |
| ---: | :--- |
|  const [**PinRef**](classPinRef.md) | [**PIN\_NC**](#variable-pin_nc)  <br>_No-connect sentinel. Pass where no physical pin exists._  |


## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint8\_t | [**PORT\_A**](#variable-port_a)   = `0`<br>_&lt; 74HC165/'595 SPI shift-register bus_  |
|  constexpr uint8\_t | [**PORT\_B**](#variable-port_b)   = `1`<br>_MCP23017 GPB port constant for constructors._  |










































## Detailed Description


Abstracts four hardware pin sources behind a single interface: direct STM32 GPIO, MCP23017 expander GPIO, [**ADS1115**](classADS1115.md) ADC channel, and a ShiftBus bit (74HC165 input / 74HC595 output over SPI). Used by all input and output classes so control declarations are identical regardless of where the physical pin lives.


Contains no logic beyond read/write dispatch — no debounce, no filtering, no state. [**PinRef**](classPinRef.md) objects are value types: live on the stack or as class members, never heap-allocated.




**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    
## Public Attributes Documentation




### variable PIN\_NC 

_No-connect sentinel. Pass where no physical pin exists._ 
```C++
const PinRef PIN_NC;
```



All reads return false / 0; all writes are no-ops. Provided as a named constant for readability in wiring maps.


Usage: `PinRef pins[] = { PinRef(exp1, PORT_A, 0) , PIN_NC, PinRef(exp1, PORT_A, 2) };`


See also: ANALOG\_NC (0xFFFF) for AnalogMultiPos unused voltage levels. 


        

<hr>
## Public Static Attributes Documentation




### variable PORT\_A 

_&lt; 74HC165/'595 SPI shift-register bus_ 
```C++
constexpr uint8_t PORT_A;
```



MCP23017 GPA port constant for constructors 


        

<hr>



### variable PORT\_B 

_MCP23017 GPB port constant for constructors._ 
```C++
constexpr uint8_t PORT_B;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PinRef.h`

