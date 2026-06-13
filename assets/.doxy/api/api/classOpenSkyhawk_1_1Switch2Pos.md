

# Class OpenSkyhawk::Switch2Pos



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md)



_Debounced 2-position switch. Self-registers into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._[More...](#detailed-description)

* `#include <Switch2Pos.h>`



Inherits the following classes: [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)


























## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint16\_t | [**DEBOUNCE\_MS**](#variable-debounce_ms)   = `20`<br> |




























## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**Switch2Pos**](#function-switch2pos-12) (uint16\_t controlId, [**PinRef**](classPinRef.md) pin) <br>_Construct a 2-position switch with default settings._  |
|   | [**Switch2Pos**](#function-switch2pos-22) (uint16\_t controlId, [**PinRef**](classPinRef.md) pin, bool reverse) <br>_Construct a 2-position switch with explicit polarity._  |
| virtual void | [**configure**](#function-configure) () override<br>_Configure the input pin. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _after chip.begin()._ |
| virtual void | [**forceReport**](#function-forcereport) () override<br>_Read current pin state and emit EVT unconditionally — no debounce._  |
| virtual void | [**poll**](#function-poll) () override<br>_Read current pin state, apply debounce, emit EVT if confirmed state changed._  |


## Public Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
| virtual void | [**configure**](classOpenSkyhawk_1_1InputBase.md#function-configure) () <br>_Configure hardware pins for this input._  |
| virtual void | [**forceReport**](classOpenSkyhawk_1_1InputBase.md#function-forcereport) () = 0<br>_Read hardware state and emit a CAN EVT unconditionally._  |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**next**](classOpenSkyhawk_1_1InputBase.md#function-next) () const<br>_Next input in the list; nullptr at end._  |
| virtual void | [**poll**](classOpenSkyhawk_1_1InputBase.md#function-poll) () = 0<br>_Read hardware state and emit a CAN EVT if state changed._  |




## Public Static Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**head**](classOpenSkyhawk_1_1InputBase.md#function-head) () <br>_Head of the self-registered linked list._  |












































## Protected Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**InputBase**](classOpenSkyhawk_1_1InputBase.md#function-inputbase) () <br>_Registers this instance into the linked list._  |






## Detailed Description


VALUE semantics (reverse = false, default): 1 — active (pin LOW — switch closed, pulling pin to GND via board pull-up) 0 — inactive (pin HIGH)


VALUE semantics (reverse = true): 1 — active (pin HIGH — switch drives pin HIGH; external pull-down holds pin LOW when open) 0 — inactive (pin LOW)


Debounce: fixed 20 ms. The pin must hold its new level for the debounce period before the state is confirmed and a CAN EVT is emitted. Any level change during the window restarts the timer.


[**forceReport()**](classOpenSkyhawk_1_1Switch2Pos.md#function-forcereport) emits the current physical state immediately without debounce — called by [**PanelGroup**](namespacePanelGroup.md) during the boot EVT burst and on SYNC\_REQ. 


    
## Public Static Attributes Documentation




### variable DEBOUNCE\_MS 

```C++
constexpr uint16_t OpenSkyhawk::Switch2Pos::DEBOUNCE_MS;
```




<hr>
## Public Functions Documentation




### function Switch2Pos [1/2]

_Construct a 2-position switch with default settings._ 
```C++
OpenSkyhawk::Switch2Pos::Switch2Pos (
    uint16_t controlId,
    PinRef pin
) 
```



Active-LOW (reverse = false), 20 ms debounce.




**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing. 
* `pin` [**PinRef**](classPinRef.md) for the switch input pin (GPIO or MCP23017). 




        

<hr>



### function Switch2Pos [2/2]

_Construct a 2-position switch with explicit polarity._ 
```C++
OpenSkyhawk::Switch2Pos::Switch2Pos (
    uint16_t controlId,
    PinRef pin,
    bool reverse
) 
```





**Parameters:**


* `controlId` DCSIN\_\* or CTRL\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing. 
* `pin` [**PinRef**](classPinRef.md) for the switch input pin (GPIO or MCP23017). 
* `reverse` false (default): active-LOW — board wiring holds HIGH, switch pulls LOW. true: active-HIGH — board wiring holds LOW, switch drives HIGH. [**configure()**](classOpenSkyhawk_1_1Switch2Pos.md#function-configure) does not enable internal pull-ups; the schematic must provide the required pull-up, pull-down, or active drive. 




        

<hr>



### function configure 

_Configure the input pin. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _after chip.begin()._
```C++
virtual void OpenSkyhawk::Switch2Pos::configure () override
```



Configures the pin as an input. Does not enable internal pull-ups; board wiring supplies the input bias. Typical [**OpenSkyhawk**](namespaceOpenSkyhawk.md) switch nets use external 10 kΩ pull-ups to +3.3V and switch to GND.




**Note:**

Must not be called from the constructor — MCP23017 register writes require the chip to be initialised first. 





        
Implements [*OpenSkyhawk::InputBase::configure*](classOpenSkyhawk_1_1InputBase.md#function-configure)


<hr>



### function forceReport 

_Read current pin state and emit EVT unconditionally — no debounce._ 
```C++
virtual void OpenSkyhawk::Switch2Pos::forceReport () override
```



Called by [**PanelGroup**](namespacePanelGroup.md) during the boot EVT burst and on SYNC\_REQ. Confirms the current reading as the baseline so subsequent [**poll()**](classOpenSkyhawk_1_1Switch2Pos.md#function-poll) calls have a valid reference. 


        
Implements [*OpenSkyhawk::InputBase::forceReport*](classOpenSkyhawk_1_1InputBase.md#function-forcereport)


<hr>



### function poll 

_Read current pin state, apply debounce, emit EVT if confirmed state changed._ 
```C++
virtual void OpenSkyhawk::Switch2Pos::poll () override
```



Called by [**PanelGroup::loop()**](namespacePanelGroup.md#function-loop) during normal operation. No-op until [**forceReport()**](classOpenSkyhawk_1_1Switch2Pos.md#function-forcereport) has been called at least once. 


        
Implements [*OpenSkyhawk::InputBase::poll*](classOpenSkyhawk_1_1InputBase.md#function-poll)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Switch2Pos.h`

