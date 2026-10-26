

# Namespace Uploader



[**Namespace List**](namespaces.md) **>** [**Uploader**](namespaceUploader.md)




















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**Io**](structUploader_1_1Io.md) <br>_How the uploader reaches the client and_ [_**PanelBridge**_](namespacePanelBridge.md) _._ |






















## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**begin**](#function-begin) (const [**Io**](structUploader_1_1Io.md) & io) <br>_Install the callbacks. Call once from SimGateway::begin()._  |
|  bool | [**busy**](#function-busy) () <br>_An upload session is open or a flash runs — the calibration session must answer BUSY._  |
|  bool | [**handles**](#function-handles) (uint8\_t type) <br>_True for the CAL\_T\_UPL\_\* request types the uploader answers._  |
|  bool | [**onBridgeLine**](#function-onbridgeline) (const char \* line, uint16\_t len, uint32\_t now) <br> |
|  void | [**onClientFrame**](#function-onclientframe) (uint32\_t now) <br>_Any verified client frame (KEEPALIVE included): the client is still there._  |
|  void | [**onFrame**](#function-onframe) (uint8\_t type, uint8\_t seq, const uint8\_t \* p, uint16\_t len, uint32\_t now) <br>_A verified client frame of one of those types._  |
|  void | [**onUartByte**](#function-onuartbyte) (uint8\_t c, uint32\_t now) <br> |
|  bool | [**ownsUart**](#function-ownsuart) () <br>_A flash runs: the uploader writes the UART, so the client's export bytes must be dropped._  |
|  bool | [**ownsUartRx**](#function-ownsuartrx) () <br> |
|  void | [**tick**](#function-tick) (uint32\_t now) <br>_Advance the flash. Call every loop()._  |




























## Public Functions Documentation




### function begin 

_Install the callbacks. Call once from SimGateway::begin()._ 
```C++
void Uploader::begin (
    const Io & io
) 
```




<hr>



### function busy 

_An upload session is open or a flash runs — the calibration session must answer BUSY._ 
```C++
bool Uploader::busy () 
```




<hr>



### function handles 

_True for the CAL\_T\_UPL\_\* request types the uploader answers._ 
```C++
bool Uploader::handles (
    uint8_t type
) 
```




<hr>



### function onBridgeLine 

```C++
bool Uploader::onBridgeLine (
    const char * line,
    uint16_t len,
    uint32_t now
) 
```



A complete line from [**PanelBridge**](namespacePanelBridge.md) (without the '
'). Returns true when the line must not be relayed to the client — `_BOOT_RSP` while the uploader itself is flashing. 


        

<hr>



### function onClientFrame 

_Any verified client frame (KEEPALIVE included): the client is still there._ 
```C++
void Uploader::onClientFrame (
    uint32_t now
) 
```




<hr>



### function onFrame 

_A verified client frame of one of those types._ 
```C++
void Uploader::onFrame (
    uint8_t type,
    uint8_t seq,
    const uint8_t * p,
    uint16_t len,
    uint32_t now
) 
```




<hr>



### function onUartByte 

```C++
void Uploader::onUartByte (
    uint8_t c,
    uint32_t now
) 
```




<hr>



### function ownsUart 

_A flash runs: the uploader writes the UART, so the client's export bytes must be dropped._ 
```C++
bool Uploader::ownsUart () 
```




<hr>



### function ownsUartRx 

```C++
bool Uploader::ownsUartRx () 
```



Flashing the bridge (#367): its bootloader answers on the UART in its own framing, so every byte [**PanelBridge**](namespacePanelBridge.md) sends goes to [**onUartByte()**](namespaceUploader.md#function-onuartbyte) instead of the line / HID parser — until the bridge reboots. 


        

<hr>



### function tick 

_Advance the flash. Call every loop()._ 
```C++
void Uploader::tick (
    uint32_t now
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/Uploader.cpp`

