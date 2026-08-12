

# Namespace SimGateway



[**Namespace List**](namespaces.md) **>** [**SimGateway**](namespaceSimGateway.md)










































## Public Functions

| Type | Name |
| ---: | :--- |
|  const [**OpenSkyhawk::CalBlob**](structOpenSkyhawk_1_1CalBlob.md) & | [**calibration**](#function-calibration) () <br>_The calibration currently applied to dispatched axis values._  |
|  void | [**loop**](#function-loop) () <br>_Relay bytes and dispatch HID frames. Call once per_ [_**loop()**_](namespaceSimGateway.md#function-loop) _iteration._ |
|  void | [**setup**](#function-setup) (SerialUART & uart, uint8\_t txPin=DEFAULT\_UART\_TX\_PIN, uint8\_t rxPin=DEFAULT\_UART\_RX\_PIN) <br>_Initialise USB identity, OpenSkyhawkJoystick, and UART link to_ [_**PanelBridge**_](namespacePanelBridge.md) _._ |
|  void | [**statusLedBegin**](#function-statusledbegin) () <br>_Configure GP2 (green) / GP3 (red) as outputs, both off. Call once from_ [_**setup()**_](namespaceSimGateway.md#function-setup) _._ |
|  void | [**statusTick**](#function-statustick) () <br>_Advance the status-LED state machine and animation. Call once per_ [_**loop()**_](namespaceSimGateway.md#function-loop) _._ |




























## Public Functions Documentation




### function calibration 

_The calibration currently applied to dispatched axis values._ 
```C++
const OpenSkyhawk::CalBlob & SimGateway::calibration () 
```





**Returns:**

The in-RAM blob, loaded by [**setup()**](namespaceSimGateway.md#function-setup) from flash.


Every axis reads as uncalibrated until [**setup()**](namespaceSimGateway.md#function-setup) loads a valid blob, so this is safe to call at any time. Use [**OpenSkyhawk::axisCalValid()**](namespaceOpenSkyhawk.md#function-axiscalvalid) on an element to ask whether that particular axis is calibrated. 


        

<hr>



### function loop 

_Relay bytes and dispatch HID frames. Call once per_ [_**loop()**_](namespaceSimGateway.md#function-loop) _iteration._
```C++
void SimGateway::loop () 
```



Per call:
* Forward all USB CDC bytes (Serial) to UART — DCS-BIOS stream to [**PanelBridge**](namespacePanelBridge.md).
* Drain all UART bytes: byte ≤ 0x7F → forward to USB CDC (DCS-BIOS from [**PanelBridge**](namespacePanelBridge.md)) 0xAA 0x55 + 4 bytes → parse controlId + value LE; dispatch to HID lists 0xAA + non-0x55 → forward 0xAA + byte to USB CDC; resume IDLE
* If any HID setter fired, call OsJoystick.send() exactly once.






**Note:**

Node-status reporting (#86) needs no handling here: [**PanelBridge**](namespacePanelBridge.md)'s `_NODE_STATUS` DCS-BIOS messages are ASCII (≤ 0x7F) forwarded by step 2, and the host's roster request is a DCS-BIOS export write (addr 0x86FE) forwarded by step 1. Both transit transparently.




**Note:**

Parser state persists across calls — frames split across iterations assemble correctly. 





        

<hr>



### function setup 

_Initialise USB identity, OpenSkyhawkJoystick, and UART link to_ [_**PanelBridge**_](namespacePanelBridge.md) _._
```C++
void SimGateway::setup (
    SerialUART & uart,
    uint8_t txPin=DEFAULT_UART_TX_PIN,
    uint8_t rxPin=DEFAULT_UART_RX_PIN
) 
```



Must be the first call in the sketch's [**setup()**](namespaceSimGateway.md#function-setup). Sets USB identity before the TinyUSB stack enumerates:
* Manufacturer: "OpenSkyhawk"
* Product: "A-4E Skyhawk"
* VID/PID: 0x2E8A / 0x4134
* CDC port: "A-4E Skyhawk DCS-BIOS" (iInterface — names the serial port) Configures the UART pins and calls uart.begin(250000), then loads the stored axis calibration and calls OsJoystick.begin() to initialise the HID descriptor and enumerate.






**Parameters:**


* `uart` Hardware UART connected to [**PanelBridge**](namespacePanelBridge.md) (Serial1 / UART0 on standard board). 
* `txPin` RP2040 UART TX pin. Defaults to GP0, wired to STM32 PA3. 
* `rxPin` RP2040 UART RX pin. Defaults to GP1, wired to STM32 PA2.



**Note:**

Calibration is loaded before the HID stack comes up, which matters only for tidiness — the load itself has no ordering requirement. Axis objects are constructed at static init, long before this runs, and they index a `.bss` blob that the C runtime has already zeroed. A zeroed blob fails axisCalValid(), so an axis dispatched before [**setup()**](namespaceSimGateway.md#function-setup) would pass through unchanged rather than divide by zero. 





        

<hr>



### function statusLedBegin 

_Configure GP2 (green) / GP3 (red) as outputs, both off. Call once from_ [_**setup()**_](namespaceSimGateway.md#function-setup) _._
```C++
void SimGateway::statusLedBegin () 
```



Called automatically by [**SimGateway::setup()**](namespaceSimGateway.md#function-setup); sketches do not call it directly. 


        

<hr>



### function statusTick 

_Advance the status-LED state machine and animation. Call once per_ [_**loop()**_](namespaceSimGateway.md#function-loop) _._
```C++
void SimGateway::statusTick () 
```



Samples USB-mount, recent DCS-BIOS activity, and the uart0 PL011 error flags, then drives GP2/GP3 for the current state. Non-blocking. Called automatically by [**SimGateway::loop()**](namespaceSimGateway.md#function-loop); sketches do not call it directly. 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/SimGateway.cpp`

