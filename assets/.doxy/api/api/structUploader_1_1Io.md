

# Struct Uploader::Io



[**ClassList**](annotated.md) **>** [**Uploader**](namespaceUploader.md) **>** [**Io**](structUploader_1_1Io.md)



_How the uploader reaches the client and_ [_**PanelBridge**_](namespacePanelBridge.md) _._

* `#include <Uploader.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  void(\* | [**ack**](#variable-ack)  <br> |
|  void(\* | [**nack**](#variable-nack)  <br> |
|  void(\* | [**send**](#variable-send)  <br>_a frame to the client_  |
|  void(\* | [**writeUart**](#variable-writeuart)  <br>_bytes to_ [_**PanelBridge**_](namespacePanelBridge.md) __ |












































## Public Attributes Documentation




### variable ack 

```C++
void(* Uploader::Io::ack) (uint8_t type, uint8_t seq);
```




<hr>



### variable nack 

```C++
void(* Uploader::Io::nack) (uint8_t type, uint8_t seq, uint8_t reason, uint8_t detail);
```




<hr>



### variable send 

_a frame to the client_ 
```C++
void(* Uploader::Io::send) (uint8_t type, uint8_t seq, const uint8_t *payload, uint16_t len);
```




<hr>



### variable writeUart 

_bytes to_ [_**PanelBridge**_](namespacePanelBridge.md) __
```C++
void(* Uploader::Io::writeUart) (const uint8_t *p, uint16_t n);
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/Uploader.h`

