

# Namespace PanelBridge



[**Namespace List**](namespaces.md) **>** [**PanelBridge**](namespacePanelBridge.md)






















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\*)(uint8\_t nodeId) | [**NodeCallback**](#typedef-nodecallback)  <br>_Callback type for node liveness events. Parameter is_ [_**PanelGroup**_](namespacePanelGroup.md) _node ID (1–63)._ |
















































## Public Types Documentation




### typedef NodeCallback 

_Callback type for node liveness events. Parameter is_ [_**PanelGroup**_](namespacePanelGroup.md) _node ID (1–63)._
```C++
using PanelBridge::NodeCallback = typedef void(*)(uint8_t nodeId);
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelBridge/PanelBridge.cpp`

