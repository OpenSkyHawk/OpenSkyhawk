

# Class PinRef



[**ClassList**](annotated.md) **>** [**PinRef**](classPinRef.md)



_Hardware pin abstraction used by all_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _input and output classes._[More...](#detailed-description)

* `#include <PinRef.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  [**ADS1115**](classADS1115.md) \* | [**adc**](#variable-adc)  <br> |
|  struct [**PinRef**](classPinRef.md) | [**ads**](#variable-ads)  <br>[_**ADS1115**_](classADS1115.md) _source._ |
|  uint8\_t | [**bit**](#variable-bit)  <br> |
|  uint8\_t | [**channel**](#variable-channel)  <br> |
|  MCP23017 \* | [**chip**](#variable-chip)  <br> |
|  struct [**PinRef**](classPinRef.md) | [**mcp**](#variable-mcp)  <br>_MCP23017 source._  |
|  uint8\_t | [**pin**](#variable-pin)  <br>_GPIO pin number._  |
|  uint8\_t | [**port**](#variable-port)  <br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**PinRef**](#function-pinref-14) (uint8\_t pin) <br>_Direct STM32 GPIO pin._  |
|   | [**PinRef**](#function-pinref-24) (MCP23017 & chip, uint8\_t port, uint8\_t bit) <br>_MCP23017 expander GPIO._  |
|   | [**PinRef**](#function-pinref-34) ([**ADS1115**](classADS1115.md) & adc, uint8\_t channel) <br>[_**ADS1115**_](classADS1115.md) _ADC channel._ |
|  constexpr | [**PinRef**](#function-pinref-44) () <br>_No-connect sentinel — represents a position with no physical pin._  |
|  void | [**configureAsInput**](#function-configureasinput) () <br>_Configure this pin as a digital input._  |
|  void | [**configureAsOutput**](#function-configureasoutput) () <br>_Configure this pin as a digital output._  |
|  uint8\_t | [**gpioPin**](#function-gpiopin) () const<br>_Return the raw Arduino pin number for GPIO PinRefs._  |
|  bool | [**isGpio**](#function-isgpio) () const<br>_Returns true if this_ [_**PinRef**_](classPinRef.md) _wraps a direct STM32 GPIO pin._ |
|  bool | [**isNC**](#function-isnc) () const<br>_Returns true if this is the NC (no-connect) sentinel._  |
|  bool | [**read**](#function-read) () const<br>_Digital read._  |
|  uint16\_t | [**readAnalog**](#function-readanalog) () const<br>_Analog read, normalised to 16-bit (0–65535)._  |
|  bool | [**readLive**](#function-readlive) () const<br>_Live digital read — bypasses any cache._  |
|  void | [**write**](#function-write) (bool value) <br>_Digital write._  |
|  void | [**writeAnalog**](#function-writeanalog) (uint16\_t val) <br>_Analog write (PWM). GPIO only._  |
|  void | [**writeDeferred**](#function-writedeferred) (bool value) <br>_Like_ [_**write()**_](classPinRef.md#function-write) _, but MCP writes only update the cache (no I2C) — the caller then invokes_[_**PanelGroup::flushExpanderWrites()**_](namespacePanelGroup.md#function-flushexpanderwrites) _to push each port in one writePort()._ |




























## Detailed Description


Provides read, write, readAnalog, and writeAnalog behind a uniform interface regardless of whether the backing pin is a direct STM32 GPIO, an MCP23017 expander bit, or an [**ADS1115**](classADS1115.md) ADC channel. Size: ~12 bytes (1-byte type tag + largest union member). 


    
## Public Attributes Documentation




### variable adc 

```C++
ADS1115* PinRef::adc;
```




<hr>



### variable ads 

[_**ADS1115**_](classADS1115.md) _source._
```C++
struct PinRef PinRef::ads;
```




<hr>



### variable bit 

```C++
uint8_t PinRef::bit;
```




<hr>



### variable channel 

```C++
uint8_t PinRef::channel;
```




<hr>



### variable chip 

```C++
MCP23017* PinRef::chip;
```




<hr>



### variable mcp 

_MCP23017 source._ 
```C++
struct PinRef PinRef::mcp;
```




<hr>



### variable pin 

_GPIO pin number._ 
```C++
uint8_t PinRef::pin;
```




<hr>



### variable port 

```C++
uint8_t PinRef::port;
```




<hr>
## Public Functions Documentation




### function PinRef [1/4]

_Direct STM32 GPIO pin._ 
```C++
explicit PinRef::PinRef (
    uint8_t pin
) 
```





**Parameters:**


* `pin` Arduino pin number (e.g. PA0, PB5). 




        

<hr>



### function PinRef [2/4]

_MCP23017 expander GPIO._ 
```C++
PinRef::PinRef (
    MCP23017 & chip,
    uint8_t port,
    uint8_t bit
) 
```





**Parameters:**


* `chip` Reference to the registered MCP23017 instance. 
* `port` PORT\_A (0) or PORT\_B (1). 
* `bit` Bit within the port, 0–7. 




        

<hr>



### function PinRef [3/4]

[_**ADS1115**_](classADS1115.md) _ADC channel._
```C++
PinRef::PinRef (
    ADS1115 & adc,
    uint8_t channel
) 
```





**Parameters:**


* `adc` Reference to the [**ADS1115**](classADS1115.md) instance. 
* `channel` Channel number, 0–3. 




        

<hr>



### function PinRef [4/4]

_No-connect sentinel — represents a position with no physical pin._ 
```C++
inline constexpr PinRef::PinRef () 
```



All reads return false / 0. All writes are no-ops. Equivalent to PIN\_NC. Provided for use in array initialisers.




**Note:**

constexpr so `PIN_NC` — and any default-constructed NC [**PinRef**](classPinRef.md) — is constant-initialized. This makes it safe to place in a global wiring-map array without the static-initialization-order hazard a dynamically-initialized global would have. 





        

<hr>



### function configureAsInput 

_Configure this pin as a digital input._ 
```C++
void PinRef::configureAsInput () 
```



GPIO: calls pinMode(pin, INPUT). Bias is provided by board wiring. MCP23017: sets IODIR bit to 1 (input) and GPPU bit to 0 (pull-up disabled) for this pin only via [**PanelGroup**](namespacePanelGroup.md)'s expander management path. [**ADS1115**](classADS1115.md): no-op — always an analog input. NC: no-op.




**Note:**

Must be called after chip.begin() — i.e. from an InputBase::configure() override called by [**PanelGroup::setup()**](namespacePanelGroup.md#function-setup), not from a constructor. 





        

<hr>



### function configureAsOutput 

_Configure this pin as a digital output._ 
```C++
void PinRef::configureAsOutput () 
```



GPIO: calls pinMode(pin, OUTPUT). MCP23017: sets IODIR bit to 0 (output) and GPPU bit to 0 (pull-up disabled) for this pin only via [**PanelGroup**](namespacePanelGroup.md)'s expander management path. [**ADS1115**](classADS1115.md): no-op; debug assertion fires if PINREF\_DEBUG is defined. NC: no-op.




**Note:**

Must be called after chip.begin() — i.e. from an OutputBase::configure() override called by [**PanelGroup::setup()**](namespacePanelGroup.md#function-setup), not from a constructor. 





        

<hr>



### function gpioPin 

_Return the raw Arduino pin number for GPIO PinRefs._ 
```C++
uint8_t PinRef::gpioPin () const
```



Used by output classes that must call APIs requiring a raw pin number, such as Servo::attach().




**Returns:**

Arduino pin number (e.g. PA0, PB9), or 0 for non-GPIO PinRefs. 




**Note:**

Debug builds log if called on non-GPIO PinRefs. 





        

<hr>



### function isGpio 

_Returns true if this_ [_**PinRef**_](classPinRef.md) _wraps a direct STM32 GPIO pin._
```C++
bool PinRef::isGpio () const
```



Used by direct-only output classes (AnalogOutput, ServoOutput) to reject MCP23017, [**ADS1115**](classADS1115.md), and NC pins at construction time. 


        

<hr>



### function isNC 

_Returns true if this is the NC (no-connect) sentinel._ 
```C++
bool PinRef::isNC () const
```




<hr>



### function read 

_Digital read._ 
```C++
bool PinRef::read () const
```



GPIO: digitalRead(pin) — true when the pin is HIGH. MCP23017: cached bit from [**PanelGroup**](namespacePanelGroup.md)'s last INTCAP or port read. No I2C. [**ADS1115**](classADS1115.md): true if [**readAnalog()**](classPinRef.md#function-readanalog) &gt; 32767 (half-scale threshold). NC: always false.




**Returns:**

true = HIGH, false = LOW. 





        

<hr>



### function readAnalog 

_Analog read, normalised to 16-bit (0–65535)._ 
```C++
uint16_t PinRef::readAnalog () const
```



GPIO: analogRead(pin) × 16 → 0–65520 (12-bit ADC scaled to 16-bit). [**ADS1115**](classADS1115.md): readADC\_SingleEnded(channel) × 2 → 0–65534 (15-bit single-ended scaled). MCP23017: always 0; debug assertion fires if PINREF\_DEBUG is defined. NC: always 0.




**Returns:**

Normalised 16-bit ADC value. 




**Note:**

Do not call from an ISR on [**ADS1115**](classADS1115.md) pins — blocks ~8 ms per conversion. 





        

<hr>



### function readLive 

_Live digital read — bypasses any cache._ 
```C++
bool PinRef::readLive () const
```



GPIO: digitalRead(pin) (already live). MCP23017: a fresh readPort() over I2C, also refreshing [**PanelGroup**](namespacePanelGroup.md)'s cache. [**ADS1115**](classADS1115.md): live [**readAnalog()**](classPinRef.md#function-readanalog) &gt; half-scale. NC: false.




**Returns:**

true = HIGH, false = LOW. 




**Note:**

For time-critical reads before [**PanelGroup::loop()**](namespacePanelGroup.md#function-loop) refreshes the cache — e.g. blocking homing on an MCP-backed sensor. Costs one I2C transaction per call on MCP pins. 





        

<hr>



### function write 

_Digital write._ 
```C++
void PinRef::write (
    bool value
) 
```



GPIO: digitalWrite(pin, value ? HIGH : LOW). MCP23017: sets the output bit via [**PanelGroup**](namespacePanelGroup.md)'s expander write path and cache. [**ADS1115**](classADS1115.md): no-op; debug assertion fires if PINREF\_DEBUG is defined. NC: no-op.




**Parameters:**


* `value` true = HIGH, false = LOW. 




        

<hr>



### function writeAnalog 

_Analog write (PWM). GPIO only._ 
```C++
void PinRef::writeAnalog (
    uint16_t val
) 
```



GPIO: analogWrite(pin, val &gt;&gt; 8) — maps 16-bit value to 8-bit duty cycle. MCP23017: no-op; debug assertion fires if PINREF\_DEBUG is defined. [**ADS1115**](classADS1115.md): no-op; debug assertion fires if PINREF\_DEBUG is defined. NC: no-op.




**Parameters:**


* `val` 16-bit value (0–65535); upper 8 bits used as PWM duty cycle. 



**Note:**

Pin must be a PWM-capable STM32 GPIO. No runtime check is performed. 





        

<hr>



### function writeDeferred 

_Like_ [_**write()**_](classPinRef.md#function-write) _, but MCP writes only update the cache (no I2C) — the caller then invokes_[_**PanelGroup::flushExpanderWrites()**_](namespacePanelGroup.md#function-flushexpanderwrites) _to push each port in one writePort()._
```C++
void PinRef::writeDeferred (
    bool value
) 
```



GPIO writes stay immediate (already cheap). Lets a multi-pin output batch its expander writes: one I2C transaction per port instead of one read-modify-write per pin.




**Parameters:**


* `value` true = HIGH, false = LOW. 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PinRef.h`

