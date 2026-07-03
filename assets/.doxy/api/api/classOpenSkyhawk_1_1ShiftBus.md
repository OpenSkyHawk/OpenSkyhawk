

# Class OpenSkyhawk::ShiftBus



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**ShiftBus**](classOpenSkyhawk_1_1ShiftBus.md)



_One shared SPI shift-register bus ('165 inputs + '595 outputs)._ [More...](#detailed-description)

* `#include <ShiftBus.h>`























## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint8\_t | [**MAX\_CHAIN**](#variable-max_chain)   = `8`<br>_chips per direction (64 in + 64 out)_  |
|  constexpr uint8\_t | [**MAX\_ISR\_CONSUMERS**](#variable-max_isr_consumers)   = `16`<br>_sampled encoders per bus_  |














## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**ShiftBus**](#function-shiftbus) (SPIClass & spi, uint8\_t sckPin, uint8\_t misoPin, uint8\_t mosiPin, uint8\_t loadPin, uint8\_t latchPin) <br>_Construct a bus. No hardware activity —_ [_**begin()**_](classOpenSkyhawk_1_1ShiftBus.md#function-begin) _runs in_[_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._ |
|  bool | [**active**](#function-active) () const<br>_Any SR pin configured on this bus? Dormant buses skip_ [_**begin()**_](classOpenSkyhawk_1_1ShiftBus.md#function-begin) _entirely._ |
|  void | [**addIsrConsumer**](#function-addisrconsumer) (void(\*)(void \*ctx) hook, void \* ctx) <br>_Register a hook called from the sampling ISR after each_ [_**transfer()**_](classOpenSkyhawk_1_1ShiftBus.md#function-transfer) _. Used by_[_**RotaryEncoder::configure()**_](classOpenSkyhawk_1_1RotaryEncoder.md#function-configure) _to auto-attach SR-pinned encoders._ |
|  void | [**begin**](#function-begin) () <br>_Claim pins + start SPI. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _for active buses only._ |
|  void | [**beginIsrSampling**](#function-beginisrsampling) (TIM\_TypeDef \* tim, uint16\_t sampleHz) <br>_Start a hardware timer that runs_ [_**transfer()**_](classOpenSkyhawk_1_1ShiftBus.md#function-transfer) _+ all registered consumer hooks every tick. Called by_[_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _when SHIFTBUS\_ISR\_HZ is defined._ |
|  bool | [**dirty**](#function-dirty) () const<br>_Pending output changes not yet shifted?_  |
|  void | [**flushNow**](#function-flushnow) () <br>[_**transfer()**_](classOpenSkyhawk_1_1ShiftBus.md#function-transfer) _immediately —_[_**StepperMotor**_](classOpenSkyhawk_1_1StepperMotor.md) _'s per-step flush path._ |
|  bool | [**isrActive**](#function-isractive) () const<br>_True once_ [_**beginIsrSampling()**_](classOpenSkyhawk_1_1ShiftBus.md#function-beginisrsampling) _has started the timer._ |
|  void | [**noteInput**](#function-noteinput) (uint8\_t chip) <br>_configureAsInput() hook: mark active, grow the '165 chain to cover_ `chip` _._ |
|  void | [**noteOutput**](#function-noteoutput) (uint8\_t chip) <br>_configureAsOutput() hook: mark active, grow the '595 chain to cover_ `chip` _._ |
|  bool | [**readBit**](#function-readbit) (uint8\_t chip, uint8\_t bit) const<br>_Cached '165 input bit — no bus traffic._  |
|  bool | [**readLiveBit**](#function-readlivebit) (uint8\_t chip, uint8\_t bit) <br>_Live '165 read — one_ [_**transfer()**_](classOpenSkyhawk_1_1ShiftBus.md#function-transfer) _, then the cached bit._ |
|  bool | [**readOutBit**](#function-readoutbit) (uint8\_t chip, uint8\_t bit) const<br>_Last written '595 output bit (_ [_**PinRef::read()**_](classPinRef.md#function-read) _on an output pin)._ |
|  void | [**transfer**](#function-transfer) () <br>_One bus transaction: LOAD pulse → full-duplex SPI of the auto-sized frame (outputs written, inputs captured) → LATCH pulse. Clears the dirty flag._  |
|  void | [**writeBit**](#function-writebit) (uint8\_t chip, uint8\_t bit, bool v) <br>_Set a '595 stage bit + mark the stage dirty. Published by the next loop-context_ [_**transfer()**_](classOpenSkyhawk_1_1ShiftBus.md#function-transfer) _/flushNow() (commit), never mid-group by the sampling ISR._ |




























## Detailed Description


Constructor stores parameters only — no SPI, no GPIO, no cross-global calls (static-init safe; the PIN\_NC lesson). All hardware work happens in [**begin()**](classOpenSkyhawk_1_1ShiftBus.md#function-begin), called by [**PanelGroup::setup()**](namespacePanelGroup.md#function-setup) for buses that have at least one configured SR pin. 


    
## Public Static Attributes Documentation




### variable MAX\_CHAIN 

_chips per direction (64 in + 64 out)_ 
```C++
constexpr uint8_t OpenSkyhawk::ShiftBus::MAX_CHAIN;
```




<hr>



### variable MAX\_ISR\_CONSUMERS 

_sampled encoders per bus_ 
```C++
constexpr uint8_t OpenSkyhawk::ShiftBus::MAX_ISR_CONSUMERS;
```




<hr>
## Public Functions Documentation




### function ShiftBus 

_Construct a bus. No hardware activity —_ [_**begin()**_](classOpenSkyhawk_1_1ShiftBus.md#function-begin) _runs in_[_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _._
```C++
OpenSkyhawk::ShiftBus::ShiftBus (
    SPIClass & spi,
    uint8_t sckPin,
    uint8_t misoPin,
    uint8_t mosiPin,
    uint8_t loadPin,
    uint8_t latchPin
) 
```





**Parameters:**


* `spi` SPI peripheral. Dedicated to this bus — the '165 QH output is never tristated, so MISO cannot be shared with another SPI reader. 
* `sckPin` SPI clock pin (must belong to `spi` — e.g. PB3 for SPI1-remap). 
* `misoPin` SPI MISO pin (← '165 QH). 
* `mosiPin` SPI MOSI pin (→ '595 DS). 
* `loadPin` '165 SH/LD̄ strobe (idles HIGH; pulsed LOW to capture inputs). 
* `latchPin` '595 STCP strobe (pulsed HIGH to publish outputs). 




        

<hr>



### function active 

_Any SR pin configured on this bus? Dormant buses skip_ [_**begin()**_](classOpenSkyhawk_1_1ShiftBus.md#function-begin) _entirely._
```C++
inline bool OpenSkyhawk::ShiftBus::active () const
```




<hr>



### function addIsrConsumer 

_Register a hook called from the sampling ISR after each_ [_**transfer()**_](classOpenSkyhawk_1_1ShiftBus.md#function-transfer) _. Used by_[_**RotaryEncoder::configure()**_](classOpenSkyhawk_1_1RotaryEncoder.md#function-configure) _to auto-attach SR-pinned encoders._
```C++
void OpenSkyhawk::ShiftBus::addIsrConsumer (
    void(*)(void *ctx) hook,
    void * ctx
) 
```




<hr>



### function begin 

_Claim pins + start SPI. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _for active buses only._
```C++
void OpenSkyhawk::ShiftBus::begin () 
```



Releases JTAG (SWJ → SWD-only; PB3/PB4 are JTDO/NJTRST) so the SPI1-remap pins are usable, configures LOAD/LATCH strobes, starts SPI, shifts an all-zeros output frame
* latch (defined '595 state as early as possible), then runs one [**transfer()**](classOpenSkyhawk_1_1ShiftBus.md#function-transfer) to prime the input cache before the forceReport() boot burst reads it. 




        

<hr>



### function beginIsrSampling 

_Start a hardware timer that runs_ [_**transfer()**_](classOpenSkyhawk_1_1ShiftBus.md#function-transfer) _+ all registered consumer hooks every tick. Called by_[_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _when SHIFTBUS\_ISR\_HZ is defined._
```C++
void OpenSkyhawk::ShiftBus::beginIsrSampling (
    TIM_TypeDef * tim,
    uint16_t sampleHz
) 
```





**Parameters:**


* `tim` Timer instance (TIM2 by default — TIM3 is backlight PWM). 
* `sampleHz` Sample rate (e.g. 1000). 



**Note:**

The ISR performs no CAN, no I2C, no allocation. Consumers read cached bits only. 





        

<hr>



### function dirty 

_Pending output changes not yet shifted?_ 
```C++
inline bool OpenSkyhawk::ShiftBus::dirty () const
```




<hr>



### function flushNow 

[_**transfer()**_](classOpenSkyhawk_1_1ShiftBus.md#function-transfer) _immediately —_[_**StepperMotor**_](classOpenSkyhawk_1_1StepperMotor.md) _'s per-step flush path._
```C++
inline void OpenSkyhawk::ShiftBus::flushNow () 
```




<hr>



### function isrActive 

_True once_ [_**beginIsrSampling()**_](classOpenSkyhawk_1_1ShiftBus.md#function-beginisrsampling) _has started the timer._
```C++
inline bool OpenSkyhawk::ShiftBus::isrActive () const
```




<hr>



### function noteInput 

_configureAsInput() hook: mark active, grow the '165 chain to cover_ `chip` _._
```C++
void OpenSkyhawk::ShiftBus::noteInput (
    uint8_t chip
) 
```




<hr>



### function noteOutput 

_configureAsOutput() hook: mark active, grow the '595 chain to cover_ `chip` _._
```C++
void OpenSkyhawk::ShiftBus::noteOutput (
    uint8_t chip
) 
```




<hr>



### function readBit 

_Cached '165 input bit — no bus traffic._ 
```C++
bool OpenSkyhawk::ShiftBus::readBit (
    uint8_t chip,
    uint8_t bit
) const
```





**Note:**

With ISR sampling active, a multi-bit consumer reading bit-by-bit can see two sample instants mixed within one poll (the ISR replaces the whole frame between reads). Debounced/hold-last consumers ([**MultiPosInput**](classOpenSkyhawk_1_1MultiPosInput.md)) absorb this; do not add an unfiltered multi-bit consumer without considering it. 





        

<hr>



### function readLiveBit 

_Live '165 read — one_ [_**transfer()**_](classOpenSkyhawk_1_1ShiftBus.md#function-transfer) _, then the cached bit._
```C++
bool OpenSkyhawk::ShiftBus::readLiveBit (
    uint8_t chip,
    uint8_t bit
) 
```




<hr>



### function readOutBit 

_Last written '595 output bit (_ [_**PinRef::read()**_](classPinRef.md#function-read) _on an output pin)._
```C++
bool OpenSkyhawk::ShiftBus::readOutBit (
    uint8_t chip,
    uint8_t bit
) const
```




<hr>



### function transfer 

_One bus transaction: LOAD pulse → full-duplex SPI of the auto-sized frame (outputs written, inputs captured) → LATCH pulse. Clears the dirty flag._ 
```C++
void OpenSkyhawk::ShiftBus::transfer () 
```



When ISR sampling is active, loop-context callers are wrapped in a short interrupt-masked critical section so the frame buffers stay single-owner. 


        

<hr>



### function writeBit 

_Set a '595 stage bit + mark the stage dirty. Published by the next loop-context_ [_**transfer()**_](classOpenSkyhawk_1_1ShiftBus.md#function-transfer) _/flushNow() (commit), never mid-group by the sampling ISR._
```C++
void OpenSkyhawk::ShiftBus::writeBit (
    uint8_t chip,
    uint8_t bit,
    bool v
) 
```



Writes land in a loop-owned stage frame; the ISR ships only the last committed frame. A multi-pin group ([**StepperMotor**](classOpenSkyhawk_1_1StepperMotor.md)'s four coils) therefore always reaches the '595 outputs as one atomic pattern, regardless of ISR timing between the writes. 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Helpers/ShiftBus/ShiftBus.h`

