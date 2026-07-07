

# File NodeStatus.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**NodeStatus**](dir_9111f7d39b5fc785aa55dffe02a55e74.md) **>** [**NodeStatus.cpp**](NodeStatus_8cpp.md)

[Go to the documentation of this file](NodeStatus_8cpp.md)


```C++
#include "NodeStatus.h"

// FaultSource intrusive registry — same pattern as OutputBase/InputBase. Platform-agnostic
// (pointers only); only STM32 nodes instantiate fault sources, but the list is harmless elsewhere.
namespace OpenSkyhawk {

FaultSource* FaultSource::_head = nullptr;
FaultSource::FaultSource() : _next(_head) { _head = this; }
FaultSource* FaultSource::head()       { return _head; }
FaultSource* FaultSource::next() const { return _next; }

NodeFaultCode aggregateFaults(const char** detailOut) {
    for (const FaultSource* s = FaultSource::head(); s; s = s->next()) {
        const NodeFaultCode code = s->faultCode();
        if (code != NodeFaultCode::NONE) {
            if (detailOut) {
                const char* d = s->faultDetail();
                *detailOut = d ? d : "";  // never hand back a null detail
            }
            return code;
        }
    }
    if (detailOut) *detailOut = "";
    return NodeFaultCode::NONE;
}

} // namespace OpenSkyhawk
```


