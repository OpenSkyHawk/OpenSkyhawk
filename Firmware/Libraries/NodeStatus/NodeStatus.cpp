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
