

# File LinkRecovery.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**LinkRecovery.cpp**](LinkRecovery_8cpp.md)

[Go to the documentation of this file](LinkRecovery_8cpp.md)


```C++
#include "LinkRecovery.h"

namespace OpenSkyhawk {

namespace {
enum LinkPhase : uint8_t {
    PHASE_HEALTHY,     // nothing pending
    PHASE_WAIT,        // unhealthy since sinceMs, not yet acted on
    PHASE_DETACHED,    // detached at sinceMs, attach pending
    PHASE_REATTACHED,  // attached at sinceMs, waiting for health
};
}

void linkRecoveryBegin(LinkRecovery& s, bool afterRecoveryReboot) {
    s.phase   = PHASE_HEALTHY;
    s.backoff = afterRecoveryReboot;
    s.sinceMs = 0;
}

LinkAction linkRecoveryStep(LinkRecovery& s, uint32_t now, bool healthy) {
    // Checked before health: the device reads unmounted while detached, so health is
    // meaningless here, and leaving the pull-up off would turn a recovery into an outage.
    if (s.phase == PHASE_DETACHED) {
        if ((uint32_t)(now - s.sinceMs) < LINK_REATTACH_GAP_MS) return LinkAction::NONE;
        s.phase   = PHASE_REATTACHED;
        s.sinceMs = now;
        return LinkAction::ATTACH;
    }

    if (healthy) {
        s.phase   = PHASE_HEALTHY;
        s.backoff = false;   // the link came back: the next failure starts the ladder afresh
        return LinkAction::NONE;
    }

    switch (s.phase) {
        case PHASE_HEALTHY:
            s.phase   = PHASE_WAIT;
            s.sinceMs = now;
            return LinkAction::NONE;

        case PHASE_WAIT: {
            const uint32_t wait = s.backoff ? LINK_BACKOFF_MS : LINK_BAD_MS;
            if ((uint32_t)(now - s.sinceMs) < wait) return LinkAction::NONE;
            s.phase   = PHASE_DETACHED;
            s.sinceMs = now;
            return LinkAction::DETACH;
        }

        case PHASE_REATTACHED:
            if ((uint32_t)(now - s.sinceMs) < LINK_REBOOT_AFTER_MS) return LinkAction::NONE;
            if (s.backoff) {
                // A reboot already failed to help; another would not either. Wait out the
                // backoff and re-attach again.
                s.phase   = PHASE_WAIT;
                s.sinceMs = now;
                return LinkAction::NONE;
            }
            // Stay in REATTACHED: the caller reboots, so there is no next tick to plan for. If
            // a test build ignores the action, the next tick asks again.
            return LinkAction::REBOOT;

        default:
            s.phase = PHASE_HEALTHY;
            return LinkAction::NONE;
    }
}

} // namespace OpenSkyhawk
```


