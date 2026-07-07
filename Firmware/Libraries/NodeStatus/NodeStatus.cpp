#include "NodeStatus.h"

// FaultSource intrusive registry — same pattern as OutputBase/InputBase. Platform-agnostic
// (pointers only); only STM32 nodes instantiate fault sources, but the list is harmless elsewhere.
namespace OpenSkyhawk {

FaultSource* FaultSource::_head = nullptr;
FaultSource::FaultSource() : _next(_head) { _head = this; }
FaultSource* FaultSource::head()       { return _head; }
FaultSource* FaultSource::next() const { return _next; }

} // namespace OpenSkyhawk
