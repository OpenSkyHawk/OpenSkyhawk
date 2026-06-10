

# Namespace SimGateway



[**Namespace List**](namespaces.md) **>** [**SimGateway**](namespaceSimGateway.md)



_&lt; Exposed so sketches can call Joystick.button() etc._ [More...](#detailed-description)






































## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**loop**](#function-loop) () <br>_Drain the UART from_ [_**PanelBridge**_](namespacePanelBridge.md) _and dispatch DIAG frames._ |
|  void | [**onDiagErr**](#function-ondiagerr) (void(\*)(uint8\_t, uint8\_t, uint8\_t) cb) <br> |
|  void | [**onDiagErr**](#function-ondiagerr) (void(\*)(uint8\_t tec, uint8\_t rec, uint8\_t flags) cb) <br>_Register a callback for DIAG\_ERR (CAN error counter) frames._  |
|  void | [**onDiagEvt**](#function-ondiagevt) (void(\*)(uint16\_t, uint16\_t, uint8\_t) cb) <br> |
|  void | [**onDiagEvt**](#function-ondiagevt) (void(\*)(uint16\_t controlId, uint16\_t value, uint8\_t nodeId) cb) <br>_Register a callback for DIAG\_EVT (sub-node input event) frames._  |
|  void | [**onDiagHb**](#function-ondiaghb) (void(\*)(uint8\_t, uint16\_t) cb) <br> |
|  void | [**onDiagHb**](#function-ondiaghb) (void(\*)(uint8\_t nodeId, uint16\_t rxCount) cb) <br>_Register a callback for DIAG\_HB (sub-node heartbeat) frames._  |
|  void | [**onDiagRtt**](#function-ondiagrtt) (void(\*)(uint16\_t, uint32\_t) cb) <br> |
|  void | [**onDiagRtt**](#function-ondiagrtt) (void(\*)(uint16\_t seq, uint32\_t sentMs) cb) <br>_Register a callback for DIAG\_RTT frames from_ [_**PanelBridge**_](namespacePanelBridge.md) _._ |
|  void | [**send**](#function-send) (uint16\_t controlId, uint16\_t value) <br>_Send a ControlPacket to_ [_**PanelBridge**_](namespacePanelBridge.md) _over UART._ |
|  void | [**setup**](#function-setup) (HardwareSerial & panelBridgePort) <br>_Initialise USB identity, Joystick, and UART link to_ [_**PanelBridge**_](namespacePanelBridge.md) _._ |




























## Detailed Description


Static singleton for the RP2040 DCS-BIOS / USB HID gateway.


Sets the USB VID/PID and product strings, initialises the Joystick HID composite device, and starts the UART link to [**PanelBridge**](namespacePanelBridge.md) at 250000 baud. In [**loop()**](namespaceSimGateway.md#function-loop), drains the UART and dispatches 8-byte DIAG frames to registered callbacks. Sending ControlPackets to [**PanelBridge**](namespacePanelBridge.md) is done via [**send()**](namespaceSimGateway.md#function-send). 


    
## Public Functions Documentation




### function loop 

_Drain the UART from_ [_**PanelBridge**_](namespacePanelBridge.md) _and dispatch DIAG frames._
```C++
void SimGateway::loop () 
```



Reads incoming bytes from panelBridgePort, re-syncs on DIAG\_MAGIC, and calls the registered onDiagRtt / onDiagHb / onDiagErr callbacks when a complete 8-byte DIAG frame is assembled. Call DcsBios::loop() in your sketch's [**loop()**](namespaceSimGateway.md#function-loop) before or after this. 


        

<hr>



### function onDiagErr 

```C++
void SimGateway::onDiagErr (
    void(*)(uint8_t, uint8_t, uint8_t) cb
) 
```




<hr>



### function onDiagErr 

_Register a callback for DIAG\_ERR (CAN error counter) frames._ 
```C++
void SimGateway::onDiagErr (
    void(*)(uint8_t tec, uint8_t rec, uint8_t flags) cb
) 
```





**Note:**

Set before calling [**setup()**](namespaceSimGateway.md#function-setup). 




**Parameters:**


* `cb` Callback: cb(tec, rec, flags). flags bits: 0x01=bus-off, 0x02=error-passive. 




        

<hr>



### function onDiagEvt 

```C++
void SimGateway::onDiagEvt (
    void(*)(uint16_t, uint16_t, uint8_t) cb
) 
```




<hr>



### function onDiagEvt 

_Register a callback for DIAG\_EVT (sub-node input event) frames._ 
```C++
void SimGateway::onDiagEvt (
    void(*)(uint16_t controlId, uint16_t value, uint8_t nodeId) cb
) 
```



Called when [**PanelBridge**](namespacePanelBridge.md) forwards a CAN input event from a sub-node. controlId and value are the raw [**PanelGroup::sendEvent()**](namespacePanelGroup.md#function-sendevent) arguments; nodeId identifies which sub-node sent the event (1 or 2). Use this to translate the event to a DCS-BIOS message via sendDcsBiosMessage() in the sketch.




**Note:**

Set before calling [**setup()**](namespaceSimGateway.md#function-setup). 




**Parameters:**


* `cb` Callback: cb(controlId, value, nodeId). 




        

<hr>



### function onDiagHb 

```C++
void SimGateway::onDiagHb (
    void(*)(uint8_t, uint16_t) cb
) 
```




<hr>



### function onDiagHb 

_Register a callback for DIAG\_HB (sub-node heartbeat) frames._ 
```C++
void SimGateway::onDiagHb (
    void(*)(uint8_t nodeId, uint16_t rxCount) cb
) 
```





**Note:**

Set before calling [**setup()**](namespaceSimGateway.md#function-setup). 




**Parameters:**


* `cb` Callback: cb(nodeId, rxCount). 




        

<hr>



### function onDiagRtt 

```C++
void SimGateway::onDiagRtt (
    void(*)(uint16_t, uint32_t) cb
) 
```




<hr>



### function onDiagRtt 

_Register a callback for DIAG\_RTT frames from_ [_**PanelBridge**_](namespacePanelBridge.md) _._
```C++
void SimGateway::onDiagRtt (
    void(*)(uint16_t seq, uint32_t sentMs) cb
) 
```



Called when [**PanelBridge**](namespacePanelBridge.md) forwards a sub-node echo back as a round-trip time measurement frame. seq matches the sequence number sent with CTRL\_TEST\_SEQ; sentMs is the PanelBridge-side timestamp when the TEST\_SEQ CAN frame was sent (use millis() - pingSentMs on the RP2040 side to compute RTT).




**Note:**

Set before calling [**setup()**](namespaceSimGateway.md#function-setup). 




**Parameters:**


* `cb` Callback: cb(seq, sentMs). 




        

<hr>



### function send 

_Send a ControlPacket to_ [_**PanelBridge**_](namespacePanelBridge.md) _over UART._
```C++
void SimGateway::send (
    uint16_t controlId,
    uint16_t value
) 
```



Packs controlId and value into a 4-byte ControlPacket struct and writes it to panelBridgePort. [**PanelBridge**](namespacePanelBridge.md) will broadcast it on CAN as a CTRL\_BCAST frame (or as a TEST\_SEQ frame if controlId == CTRL\_TEST\_SEQ).




**Parameters:**


* `controlId` Identifies the target control (HID, DCS-BIOS address, or CTRL\_TEST\_SEQ). 
* `value` Value to send. 




        

<hr>



### function setup 

_Initialise USB identity, Joystick, and UART link to_ [_**PanelBridge**_](namespacePanelBridge.md) _._
```C++
void SimGateway::setup (
    HardwareSerial & panelBridgePort
) 
```



Must be called before DcsBios::setup() so the USB identity is set before the composite device stack starts. Sets:
* Manufacturer: "OpenSkyhawk", Product: "A-4E Skyhawk"
* VID/PID: 0x2E8A / 0x4134






**Parameters:**


* `panelBridgePort` UART port connected to the [**PanelBridge**](namespacePanelBridge.md) STM32. Use Serial1 (GP0 TX / GP1 RX) on the standard gateway board. 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/SimGateway.cpp`

