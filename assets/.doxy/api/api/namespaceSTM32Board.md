

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
|  CAN\_HandleTypeDef \* | [**canHandle**](#function-canhandle) () <br>_Access the HAL CAN handle._  |
|  HardwareSerial & | [**diagSerial**](#function-diagserial) () <br>_Access DiagSerial directly for multi-field formatted output._  |
|  bool | [**isDebug**](#function-isdebug) () <br>_Returns true when debug output is enabled._  |
|  void | [**log**](#function-log) (const char \* msg) <br>_Print a line to DiagSerial if debug is enabled; no-op otherwise._  |
|  void | [**logNodeFaultEdge**](#function-lognodefaultedge) (const char \* tag, [**NodeFaultCode**](NodeStatus_8h.md#enum-nodefaultcode) fault, const char \* detail) <br>_Edge-log a node's fault transition to DiagSerial (#163)._  |
|  void | [**onCanStatus**](#function-oncanstatus) ([**CanStatus**](CANProtocol_8h.md#enum-canstatus) status) <br>_CAN bus status event handler — maps CanStatus to LED state._  |
|  int8\_t | [**readDieTempC**](#function-readdietempc) () <br>_Read the MCU's built-in internal temperature sensor (ADC ch16)._  |
|  uint16\_t | [**readVddMv**](#function-readvddmv) () <br>_Estimate MCU Vdd from the internal reference (Vrefint, ADC ch17)._  |
|  void | [**setDebug**](#function-setdebug) (bool on) <br>_Enable or disable DiagSerial output._  |
|  void | [**setLinkActive**](#function-setlinkactive) (bool active) <br>_Signal that application data is actively flowing → CONNECTED (green solid)._  |
|  void | [**setWarning**](#function-setwarning) (bool on=true) <br>_Raise or clear the WARNING condition — red/green alternating at 500 ms._  |
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



### function logNodeFaultEdge 

_Edge-log a node's fault transition to DiagSerial (#163)._ 
```C++
void STM32Board::logNodeFaultEdge (
    const char * tag,
    NodeFaultCode fault,
    const char * detail
) 
```



Prints `[<tag>] degraded: <detail> (fault N)` on entering or changing a fault, and `[<tag>] recovered` on clearing — only on a change, and only when [**isDebug()**](namespaceSTM32Board.md#function-isdebug) (no formatting cost otherwise). Shared by every node's health-TX loop ([**PanelGroup**](namespacePanelGroup.md), [**PanelBridge**](namespacePanelBridge.md), future PDU).


Holds its own static prev-fault state. A firmware binary has exactly ONE node identity (a build is either a [**PanelGroup**](namespacePanelGroup.md) node or the bridge), so a single static is correct — the node's aggregated fault, not per-source. Detail strings stay local — never on the CAN wire.




**Parameters:**


* `tag` Short node tag for the log line, e.g. "NODE" or "BRIDGE". 
* `fault` The node's current aggregated NodeFaultCode (from [**OpenSkyhawk::aggregateFaults()**](namespaceOpenSkyhawk.md#function-aggregatefaults)). 
* `detail` That fault's faultDetail() (non-null; ignored when fault == NONE). 




        

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



### function readDieTempC 

_Read the MCU's built-in internal temperature sensor (ADC ch16)._ 
```C++
int8_t STM32Board::readDieTempC () 
```



Free per-node thermal telemetry — no external parts, no PCB change. Reads ATEMP and AVREF (Vrefint) and converts with STM32F103 datasheet typicals (V25 = 1.43 V, Avg\_Slope = 4.3 mV/°C), referencing Vsense to the measured Vdd.




**Note:**

UNCALIBRATED: no factory trim → ~±few °C absolute; measures DIE temperature (not ambient) with a self-heat offset. Use for relative trend / overheat flagging, not precise ambient measurement. 




**Returns:**

Die temperature in whole °C, or INT8\_MIN if the internal channels are unavailable on this variant. 





        

<hr>



### function readVddMv 

_Estimate MCU Vdd from the internal reference (Vrefint, ADC ch17)._ 
```C++
uint16_t STM32Board::readVddMv () 
```





**Note:**

Uses the STM32F103 typical Vrefint of 1.20 V (the F103 has no VREFINT\_CAL factory value), so absolute accuracy is limited; good for relative trend. 




**Returns:**

Vdd in millivolts, or 0 if the internal reference is unavailable. 





        

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



### function setLinkActive 

_Signal that application data is actively flowing → CONNECTED (green solid)._ 
```C++
void STM32Board::setLinkActive (
    bool active
) 
```



Call setLinkActive(true) on each unit of inbound data ([**PanelBridge**](namespacePanelBridge.md): a DCS-BIOS export seen; [**PanelGroup**](namespacePanelGroup.md): a CTRL\_BCAST received). The link auto-decays back to NORMAL after ~500 ms with no further calls. CONNECTED is only shown while the CAN bus is healthy; a CAN fault masks it and it re-engages automatically on recovery if data is still flowing.




**Parameters:**


* `active` True to (re)assert the data-flowing link; false to drop it immediately. 




        

<hr>



### function setWarning 

_Raise or clear the WARNING condition — red/green alternating at 500 ms._ 
```C++
void STM32Board::setWarning (
    bool on=true
) 
```



Call when a degraded condition is detected that is not represented by CanStatus (e.g. SYNC timeout, dead [**PanelGroup**](namespacePanelGroup.md) node, lost master heartbeat). WARNING outranks CONNECTED/NORMAL but is masked by any CAN fault (CAN\_ERROR/BUS\_OFF). Clear it with setWarning(false) once the condition recovers.




**Parameters:**


* `on` True to raise WARNING (default); false to clear it. 




        

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

