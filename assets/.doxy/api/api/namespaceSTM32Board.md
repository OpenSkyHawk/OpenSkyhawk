

# Namespace STM32Board



[**Namespace List**](namespaces.md) **>** [**STM32Board**](namespaceSTM32Board.md)




























## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint8\_t | [**PIN\_LED\_GREEN**](#variable-pin_led_green)   = `PB15`<br>_Green LED pin — same on all STM32 boards._  |
|  constexpr uint8\_t | [**PIN\_LED\_RED**](#variable-pin_led_red)   = `PB14`<br>_Red LED pin — same on all STM32 boards._  |














## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**begin**](#function-begin) () <br>_Initialise all shared hardware. Call once at the top of setup()._  |
|  bool | [**busOff**](#function-busoff) () <br> |
|  CAN\_HandleTypeDef \* | [**canHandle**](#function-canhandle) () <br>_Access the HAL CAN handle._  |
|  bool | [**canSend**](#function-cansend) (uint32\_t canId, const uint8\_t \* data, uint8\_t len) <br> |
|  void | [**canStart**](#function-canstart) () <br> |
|  HardwareSerial & | [**diagSerial**](#function-diagserial) () <br>_Access DiagSerial directly for multi-field formatted output._  |
|  bool | [**isDebug**](#function-isdebug) () <br>_Returns true when debug output is enabled._  |
|  void | [**log**](#function-log) (const char \* msg) <br>_Print a line to DiagSerial if debug is enabled; no-op otherwise._  |
|  void | [**onCanStatus**](#function-oncanstatus) ([**CanStatus**](CANProtocol_8h.md#enum-canstatus) status) <br>_CAN bus status event handler — maps CanStatus to LED state._  |
|  uint8\_t | [**rec**](#function-rec) () <br> |
|  void | [**setDebug**](#function-setdebug) (bool on) <br>_Enable or disable DiagSerial output._  |
|  void | [**setWarning**](#function-setwarning) () <br>_Enter WARNING LED state — red/green alternating at 500 ms._  |
|  uint8\_t | [**tec**](#function-tec) () <br> |
|  void | [**tick**](#function-tick) () <br>_Drive LED animations. Call once per loop() iteration._  |




























## Public Static Attributes Documentation




### variable PIN\_LED\_GREEN 

_Green LED pin — same on all STM32 boards._ 
```C++
constexpr uint8_t STM32Board::PIN_LED_GREEN;
```




<hr>



### variable PIN\_LED\_RED 

_Red LED pin — same on all STM32 boards._ 
```C++
constexpr uint8_t STM32Board::PIN_LED_RED;
```




<hr>
## Public Functions Documentation




### function begin 

_Initialise all shared hardware. Call once at the top of setup()._ 
```C++
void STM32Board::begin () 
```



Configures PB14 (Red) and PB15 (Green) as outputs, enters BOOTING LED state. Starts DiagSerial (USART1 PA9/PA10, 115200 baud) — silent until setDebug(true). Configures the CAN peripheral at 500 kbps on PA11/PA12 but does NOT start it — call [**CANProtocol::start()**](namespaceCANProtocol.md#function-start) after filter setup. 


        

<hr>



### function busOff 

```C++
bool STM32Board::busOff () 
```





**Deprecated**

Use [**CANProtocol::busOff()**](namespaceCANProtocol.md#function-busoff) instead. 




        

<hr>



### function canHandle 

_Access the HAL CAN handle._ 
```C++
CAN_HandleTypeDef * STM32Board::canHandle () 
```





**Note:**

Internal — used by [**CANProtocol**](namespaceCANProtocol.md) only. Do not call from sketches. 




**Returns:**

Pointer to the internal CAN\_HandleTypeDef. 





        

<hr>



### function canSend 

```C++
bool STM32Board::canSend (
    uint32_t canId,
    const uint8_t * data,
    uint8_t len
) 
```





**Deprecated**

Use [**CANProtocol::send()**](namespaceCANProtocol.md#function-send) instead. 




        

<hr>



### function canStart 

```C++
void STM32Board::canStart () 
```





**Deprecated**

Use [**CANProtocol::start()**](namespaceCANProtocol.md#function-start) instead. 




        

<hr>



### function diagSerial 

_Access DiagSerial directly for multi-field formatted output._ 
```C++
HardwareSerial & STM32Board::diagSerial () 
```



Guard with [**isDebug()**](namespaceSTM32Board.md#function-isdebug) to avoid formatting overhead when debug is off.




**Returns:**

Reference to the USART1 HardwareSerial instance. 





        

<hr>



### function isDebug 

_Returns true when debug output is enabled._ 
```C++
bool STM32Board::isDebug () 
```



Guard multi-field formatted print blocks with this to skip string formatting overhead when debug is off. 


        

<hr>



### function log 

_Print a line to DiagSerial if debug is enabled; no-op otherwise._ 
```C++
void STM32Board::log (
    const char * msg
) 
```





**Parameters:**


* `msg` Null-terminated string to print. 




        

<hr>



### function onCanStatus 

_CAN bus status event handler — maps CanStatus to LED state._ 
```C++
void STM32Board::onCanStatus (
    CanStatus status
) 
```



Register with CANProtocol::onStatusChange(STM32Board::onCanStatus) in setup(). Never call directly from sketch code.




**Parameters:**


* `status` New CAN bus status reported by [**CANProtocol**](namespaceCANProtocol.md). 




        

<hr>



### function rec 

```C++
uint8_t STM32Board::rec () 
```





**Deprecated**

Use [**CANProtocol::rec()**](namespaceCANProtocol.md#function-rec) instead. 




        

<hr>



### function setDebug 

_Enable or disable DiagSerial output._ 
```C++
void STM32Board::setDebug (
    bool on
) 
```



DiagSerial is always initialised by [**begin()**](namespaceSTM32Board.md#function-begin); this flag gates all [**log()**](namespaceSTM32Board.md#function-log) calls.




**Parameters:**


* `on` True to emit output on USART1; false for silence (default). 




        

<hr>



### function setWarning 

_Enter WARNING LED state — red/green alternating at 500 ms._ 
```C++
void STM32Board::setWarning () 
```



Call when a degraded condition is detected that is not represented by CanStatus (e.g. SYNC timeout, missing heartbeat, application-layer fault). 


        

<hr>



### function tec 

```C++
uint8_t STM32Board::tec () 
```





**Deprecated**

Use [**CANProtocol::tec()**](namespaceCANProtocol.md#function-tec) instead. 




        

<hr>



### function tick 

_Drive LED animations. Call once per loop() iteration._ 
```C++
void STM32Board::tick () 
```



Advances blink state using millis(). Fully non-blocking. 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/STM32Board/STM32Board.cpp`

