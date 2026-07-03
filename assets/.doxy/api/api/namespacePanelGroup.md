

# Namespace PanelGroup



[**Namespace List**](namespaces.md) **>** [**PanelGroup**](namespacePanelGroup.md)



_Static singleton for CAN sub-node (_ [_**PanelGroup**_](namespacePanelGroup.md) _) firmware._[More...](#detailed-description)






































## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**flushExpanderWrites**](#function-flushexpanderwrites) () <br>_Push every port dirtied by_ [_**writeCachedPinDeferred()**_](namespacePanelGroup.md#function-writecachedpindeferred) _— one writePort() each._ |
|  void | [**loop**](#function-loop) () <br>_Run all_ [_**PanelGroup**_](namespacePanelGroup.md) _work. Call once per_[_**loop()**_](namespacePanelGroup.md#function-loop) _iteration._ |
|  void | [**noteShiftBus**](#function-noteshiftbus) ([**OpenSkyhawk::ShiftBus**](classOpenSkyhawk_1_1ShiftBus.md) & bus) <br>_Collect a ShiftBus at configure time. Called by_ [_**PinRef::configureAsInput()**_](classPinRef.md#function-configureasinput) _/ configureAsOutput() on SR pins — never by sketches (zero-setup lifecycle)._ |
|  bool | [**readCachedPin**](#function-readcachedpin) (const MCP23017 & chip, uint8\_t port, uint8\_t bit) <br>_Return cached MCP23017 pin state. Called by_ [_**PinRef::read()**_](classPinRef.md#function-read) _. No I2C._ |
|  bool | [**readLivePin**](#function-readlivepin) (MCP23017 & chip, uint8\_t port, uint8\_t bit) <br>_Live MCP23017 pin read — fresh readPort() over I2C, refreshing the cache._  |
|  void | [**registerADC**](#function-registeradc) ([**ADS1115**](classADS1115.md) & adc, uint8\_t addr=0x48, TwoWire & wire=Wire) <br>_Register an_ [_**ADS1115**_](classADS1115.md) _ADC. Call before_[_**setup()**_](namespacePanelGroup.md#function-setup) _._ |
|  void | [**registerExpander**](#function-registerexpander) (MCP23017 & chip, uint8\_t intaPin, uint8\_t intbPin) <br>_Register an MCP23017 expander in interrupt-driven mode. Call before_ [_**setup()**_](namespacePanelGroup.md#function-setup) _._ |
|  void | [**registerExpander**](#function-registerexpander) (MCP23017 & chip) <br>_Register an MCP23017 expander in polling-fallback mode. Call before_ [_**setup()**_](namespacePanelGroup.md#function-setup) _._ |
|  void | [**setup**](#function-setup) () <br>_Initialise_ [_**PanelGroup**_](namespacePanelGroup.md) _. Call from sketch_[_**setup()**_](namespacePanelGroup.md#function-setup) _after Wire.begin()._ |
|  void | [**writeCachedPin**](#function-writecachedpin) (MCP23017 & chip, uint8\_t port, uint8\_t bit, bool value) <br>_Write MCP23017 pin and update cache. Called by_ [_**PinRef::write()**_](classPinRef.md#function-write) _._ |
|  void | [**writeCachedPinDeferred**](#function-writecachedpindeferred) (MCP23017 & chip, uint8\_t port, uint8\_t bit, bool value) <br>_Deferred MCP23017 write — update the cache + mark the port dirty, no I2C._  |




























## Detailed Description


Manages MCP23017 expander registration and cache, [**ADS1115**](classADS1115.md) registration, the 8-step boot sequence, per-loop interrupt dispatch and polling fallback, CAN EVT batching, CTRL\_BCAST dispatch, SYNC\_REQ response, and the 500 ms HB\_n heartbeat. 


    
## Public Functions Documentation




### function flushExpanderWrites 

_Push every port dirtied by_ [_**writeCachedPinDeferred()**_](namespacePanelGroup.md#function-writecachedpindeferred) _— one writePort() each._
```C++
void PanelGroup::flushExpanderWrites () 
```





**Note:**

No-op when nothing is pending (GPIO-only outputs never dirty a port). 





        

<hr>



### function loop 

_Run all_ [_**PanelGroup**_](namespacePanelGroup.md) _work. Call once per_[_**loop()**_](namespacePanelGroup.md#function-loop) _iteration._
```C++
void PanelGroup::loop () 
```



Each call:
* Check interrupt flags; read INTCAP; update expander port caches.
* Polling fallback (~20 ms): read ports for chips registered without interrupt.
* poll() on all InputBase objects.
* [**CANProtocol::drain()**](namespaceCANProtocol.md#function-drain) — dispatches CTRL\_BCAST, SYNC\_REQ, TEST\_SEQ echo.
* update() on all OutputBase objects.
* Heartbeat: send HB\_n every 500 ms. 




        

<hr>



### function noteShiftBus 

_Collect a ShiftBus at configure time. Called by_ [_**PinRef::configureAsInput()**_](classPinRef.md#function-configureasinput) _/ configureAsOutput() on SR pins — never by sketches (zero-setup lifecycle)._
```C++
void PanelGroup::noteShiftBus (
    OpenSkyhawk::ShiftBus & bus
) 
```



Deduplicated. [**setup()**](namespacePanelGroup.md#function-setup) begin()s every collected bus after step 3; [**loop()**](namespacePanelGroup.md#function-loop) transfers them each iteration; [**flushExpanderWrites()**](namespacePanelGroup.md#function-flushexpanderwrites) flushes dirty ones. 


        

<hr>



### function readCachedPin 

_Return cached MCP23017 pin state. Called by_ [_**PinRef::read()**_](classPinRef.md#function-read) _. No I2C._
```C++
bool PanelGroup::readCachedPin (
    const MCP23017 & chip,
    uint8_t port,
    uint8_t bit
) 
```





**Parameters:**


* `chip` Chip reference — used as key to locate the ExpanderEntry. 
* `port` PORT\_A (0) or PORT\_B (1). 
* `bit` Bit index 0–7. 



**Returns:**

Cached logical level (true = HIGH). 





        

<hr>



### function readLivePin 

_Live MCP23017 pin read — fresh readPort() over I2C, refreshing the cache._ 
```C++
bool PanelGroup::readLivePin (
    MCP23017 & chip,
    uint8_t port,
    uint8_t bit
) 
```





**Parameters:**


* `chip` Chip reference. 
* `port` PORT\_A (0) or PORT\_B (1). 
* `bit` Bit index 0–7. 



**Returns:**

Live logical level (true = HIGH). 




**Note:**

Called by [**PinRef::readLive()**](classPinRef.md#function-readlive) for time-critical reads before [**loop()**](namespacePanelGroup.md#function-loop) refreshes the cache (e.g. blocking homing on an MCP-backed home sensor). One I2C transaction per call. 





        

<hr>



### function registerADC 

_Register an_ [_**ADS1115**_](classADS1115.md) _ADC. Call before_[_**setup()**_](namespacePanelGroup.md#function-setup) _._
```C++
void PanelGroup::registerADC (
    ADS1115 & adc,
    uint8_t addr=0x48,
    TwoWire & wire=Wire
) 
```



[**PanelGroup**](namespacePanelGroup.md) calls adc.begin(addr, wire) during [**setup()**](namespacePanelGroup.md#function-setup). Register each [**ADS1115**](classADS1115.md) instance exactly once — multiple AnalogInput objects may share the same chip.


Adafruit\_ADS1115 takes address and bus via begin(), not the constructor. Pattern: 
```C++
ADS1115 adc;
PanelGroup::registerADC(adc, 0x48, Wire);   // 0x48–0x4B via ADDR pin
```





**Parameters:**


* `adc` [**ADS1115**](classADS1115.md) instance. Must outlive [**PanelGroup**](namespacePanelGroup.md). 
* `addr` I2C address (0x48–0x4B via ADDR pin). Default 0x48. 
* `wire` I2C bus. Default Wire (I2C1 on STM32). 



**Note:**

Wire.begin() (or Wire1.begin()) must be called by the sketch before [**setup()**](namespacePanelGroup.md#function-setup). 





        

<hr>



### function registerExpander 

_Register an MCP23017 expander in interrupt-driven mode. Call before_ [_**setup()**_](namespacePanelGroup.md#function-setup) _._
```C++
void PanelGroup::registerExpander (
    MCP23017 & chip,
    uint8_t intaPin,
    uint8_t intbPin
) 
```



[**PanelGroup**](namespacePanelGroup.md) calls chip.init(), configures IOCON (MIRROR and/or open-drain as detected), enables interrupt-on-change on input pins only (IODIR-masked, bit 7 excluded per GPA7/GPB7 silicon erratum), reads baseline port state, and attaches STM32 ISRs.


Pass the same STM32 pin for intaPin and intbPin to use MIRROR mode (IOCON.MIRROR=1), where either port interrupt asserts the shared line.


If two or more chips share an interrupt line (wired-OR), [**PanelGroup**](namespacePanelGroup.md) sets IOCON.ODR=1 (open-drain) automatically on all chips on that line.




**Parameters:**


* `chip` blemasle/MCP23017 instance. Must outlive [**PanelGroup**](namespacePanelGroup.md). 
* `intaPin` STM32 GPIO pin connected to chip INTA. 
* `intbPin` STM32 GPIO pin connected to chip INTB. Same as intaPin for MIRROR. 



**Note:**

Do not use PB14/PB15 (status LED) or PC13–PC15 (RTC/oscillator). 





        

<hr>



### function registerExpander 

_Register an MCP23017 expander in polling-fallback mode. Call before_ [_**setup()**_](namespacePanelGroup.md#function-setup) _._
```C++
void PanelGroup::registerExpander (
    MCP23017 & chip
) 
```



[**PanelGroup**](namespacePanelGroup.md) reads all port registers every ~20 ms in [**loop()**](namespacePanelGroup.md#function-loop). No STM32 interrupt pin is required. Use when interrupt lines are not wired or not needed.




**Parameters:**


* `chip` blemasle/MCP23017 instance. Must outlive [**PanelGroup**](namespacePanelGroup.md). 




        

<hr>



### function setup 

_Initialise_ [_**PanelGroup**_](namespacePanelGroup.md) _. Call from sketch_[_**setup()**_](namespacePanelGroup.md#function-setup) _after Wire.begin()._
```C++
void PanelGroup::setup () 
```



Performs the 8-step boot sequence:
* [**STM32Board::begin()**](namespaceSTM32Board.md#function-begin)
* For each registered ADC: begin(addr, wire). For each registered expander: init(), configure IOCON, enable interrupt-on-change on input pins, read baseline port state, attach STM32 ISR.
* configure() on all InputBase and OutputBase objects.
* Register CAN callbacks.
* [**CANProtocol::start()**](namespaceCANProtocol.md#function-start).
* forceReport() burst — one EVT per registered input.
* flushBatched(canIdEvt(NODE\_ID)).
* READY\_n frame (canIdReady(NODE\_ID), DLC=0). Arm heartbeat timer.






**Note:**

Wire.begin() must be called by the sketch before this function. [**PanelGroup**](namespacePanelGroup.md) never calls Wire.begin() — only start buses in use. 





        

<hr>



### function writeCachedPin 

_Write MCP23017 pin and update cache. Called by_ [_**PinRef::write()**_](classPinRef.md#function-write) _._
```C++
void PanelGroup::writeCachedPin (
    MCP23017 & chip,
    uint8_t port,
    uint8_t bit,
    bool value
) 
```





**Parameters:**


* `chip` Chip reference. 
* `port` PORT\_A (0) or PORT\_B (1). 
* `bit` Bit index 0–7. 
* `value` Logical level to write. 




        

<hr>



### function writeCachedPinDeferred 

_Deferred MCP23017 write — update the cache + mark the port dirty, no I2C._ 
```C++
void PanelGroup::writeCachedPinDeferred (
    MCP23017 & chip,
    uint8_t port,
    uint8_t bit,
    bool value
) 
```





**Note:**

Pair with [**flushExpanderWrites()**](namespacePanelGroup.md#function-flushexpanderwrites). Lets a multi-pin output (e.g. a stepper's four coils) collapse N per-pin read-modify-writes into one writePort() per port. 





        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PanelGroup.cpp`

