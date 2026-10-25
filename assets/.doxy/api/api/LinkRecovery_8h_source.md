

# File LinkRecovery.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**LinkRecovery.h**](LinkRecovery_8h.md)

[Go to the documentation of this file](LinkRecovery_8h.md)


```C++

#pragma once
#include <stdint.h>

namespace OpenSkyhawk {

constexpr uint32_t LINK_BAD_MS          = 5000;   
constexpr uint32_t LINK_REATTACH_GAP_MS = 100;    
constexpr uint32_t LINK_REBOOT_AFTER_MS = 5000;   
constexpr uint32_t LINK_BACKOFF_MS      = 60000;  

enum class LinkAction : uint8_t {
    NONE,    
    DETACH,  
    ATTACH,  
    REBOOT,  
};

struct LinkRecovery {
    uint8_t  phase;      // internal: LinkPhase
    bool     backoff;    // a recovery reboot already happened and has not been followed by health
    uint32_t sinceMs;    // start of the current phase
};

void linkRecoveryBegin(LinkRecovery& s, bool afterRecoveryReboot);

LinkAction linkRecoveryStep(LinkRecovery& s, uint32_t now, bool healthy);

} // namespace OpenSkyhawk
```


