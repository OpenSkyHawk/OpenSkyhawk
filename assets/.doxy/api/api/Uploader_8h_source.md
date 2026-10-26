

# File Uploader.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**Uploader.h**](Uploader_8h.md)

[Go to the documentation of this file](Uploader_8h.md)


```C++

#pragma once

#include <stdint.h>

namespace Uploader {

struct Io {
    void (*send)(uint8_t type, uint8_t seq, const uint8_t* payload, uint16_t len);   
    void (*ack)(uint8_t type, uint8_t seq);
    void (*nack)(uint8_t type, uint8_t seq, uint8_t reason, uint8_t detail);
    void (*writeUart)(const uint8_t* p, uint16_t n);                                  
};

void begin(const Io& io);

bool handles(uint8_t type);

void onFrame(uint8_t type, uint8_t seq, const uint8_t* payload, uint16_t len, uint32_t now);

void onClientFrame(uint32_t now);

bool onBridgeLine(const char* line, uint16_t len, uint32_t now);

void tick(uint32_t now);

bool busy();

bool ownsUart();

bool ownsUartRx();
void onUartByte(uint8_t b, uint32_t now);

}  // namespace Uploader
```


