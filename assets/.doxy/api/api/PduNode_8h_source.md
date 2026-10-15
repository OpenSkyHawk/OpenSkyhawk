

# File PduNode.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**PduNode.h**](PduNode_8h.md)

[Go to the documentation of this file](PduNode_8h.md)


```C++

#pragma once
#include <stdint.h>
#include <STM32Board.h>
#include <CANProtocol.h>
#include "RailMonitor.h"
#include "BoardThermal.h"
#include "PduCalStore.h"

namespace PduNode {

void setup();

void loop();

uint16_t scanRateHz();

const OpenSkyhawk::RailMonitor& rail12();
const OpenSkyhawk::RailMonitor& rail5();
const OpenSkyhawk::BoardThermal& thermal();

} // namespace PduNode
```


