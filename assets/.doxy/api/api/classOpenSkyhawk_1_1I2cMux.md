

# Class OpenSkyhawk::I2cMux



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**I2cMux**](classOpenSkyhawk_1_1I2cMux.md)



_Selects one downstream channel of a TCA9548A I2C multiplexer._ [More...](#detailed-description)

* `#include <I2cMux.h>`





































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**I2cMux**](#function-i2cmux) (uint8\_t addr=0x70, TwoWire & wire=Wire) <br>_Construct a mux handle. No I2C occurs here._  |
|  bool | [**deviceAcks**](#function-deviceacks) (uint8\_t addr7) <br>_Probe a downstream device on the CURRENTLY SELECTED channel — does it ACK?_  |
|  void | [**disableAll**](#function-disableall) () <br>_Disable all channels (control byte 0x00). Optional bus quiescing._  |
|  bool | [**select**](#function-select) (uint8\_t channel, bool force=false) <br>_Route the bus to one downstream channel._  |




























## Detailed Description


Stateless beyond a last-selected cache: select(ch) writes the 1-of-8 channel bitmask to the TCA9548A control register only when the requested channel differs from the last one written, so repeated [**select()**](classOpenSkyhawk_1_1I2cMux.md#function-select) of the same channel costs no I2C. Construct one per physical TCA9548A. The sketch owns Wire.begin(); [**I2cMux**](classOpenSkyhawk_1_1I2cMux.md) never starts the bus and performs no I2C in its constructor. 


    
## Public Functions Documentation




### function I2cMux 

_Construct a mux handle. No I2C occurs here._ 
```C++
explicit OpenSkyhawk::I2cMux::I2cMux (
    uint8_t addr=0x70,
    TwoWire & wire=Wire
) 
```





**Parameters:**


* `addr` TCA9548A 7-bit I2C address (0x70–0x77 via A0/A1/A2). Default 0x70. 
* `wire` I2C bus the mux sits on. Default Wire (I2C1 on STM32). 




        

<hr>



### function deviceAcks 

_Probe a downstream device on the CURRENTLY SELECTED channel — does it ACK?_ 
```C++
bool OpenSkyhawk::I2cMux::deviceAcks (
    uint8_t addr7
) 
```





**Parameters:**


* `addr7` 7-bit address of the device behind the selected branch. 



**Returns:**

true if the device ACKs. Call select(channel) first to route the bus to it. 





        

<hr>



### function disableAll 

_Disable all channels (control byte 0x00). Optional bus quiescing._ 
```C++
void OpenSkyhawk::I2cMux::disableAll () 
```




<hr>



### function select 

_Route the bus to one downstream channel._ 
```C++
bool OpenSkyhawk::I2cMux::select (
    uint8_t channel,
    bool force=false
) 
```





**Parameters:**


* `channel` Channel 0–7. Values above 7 are clamped to 7. 
* `force` Write the channel byte even if it matches the cache. Use on health/recovery paths: a TCA9548A that reset / power-glitched comes back with no channel selected while the cache still matches, so a plain [**select()**](classOpenSkyhawk_1_1I2cMux.md#function-select) would skip the write and the branch would stay dark forever. 



**Returns:**

true if the channel is selected (write ACKed, or already current and not forced); false on I2C NAK — also invalidates the cache so the next select re-routes. 




**Note:**

Writes a single byte (1 &lt;&lt; channel); skipped when channel == last selected unless forced. Callers sharing one mux across several devices MUST call this immediately before each downstream I2C op — an interleaved driver can change the channel. 





        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Helpers/I2cMux/I2cMux.h`

