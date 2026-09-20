

# Class OpenSkyhawk::ActionButton



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**ActionButton**](classOpenSkyhawk_1_1ActionButton.md)



_Momentary push button driving a control that latches in the sim. Self-registers into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._[More...](#detailed-description)

* `#include <ActionButton.h>`



Inherits the following classes: [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)


























## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint32\_t | [**DEBOUNCE\_MS**](#variable-debounce_ms)   = `20`<br> |




























## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**ActionButton**](#function-actionbutton) (uint16\_t controlId, [**PinRef**](classPinRef.md) pin, bool reverse=false) <br>_Construct a momentary action button._  |
| virtual void | [**configure**](#function-configure) () override<br>_Configure the input pin. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _after chip.begin()._ |
| virtual void | [**forceReport**](#function-forcereport) () override<br>_Establish the current pin state as the baseline._ **Emits nothing.** __ |
| virtual void | [**poll**](#function-poll) () override<br>_Read the pin and emit one EVT\_ACTION on a debounced press edge._  |


## Public Functions inherited from OpenSkyhawk::InputBase

See [OpenSkyhawk::InputBase](classOpenSkyhawk_1_1InputBase.md)

| Type | Name |
| ---: | :--- |
| virtual void | [**configure**](classOpenSkyhawk_1_1InputBase.md#function-configure) () <br>_Configure hardware pins for this input._  |
| virtual void | [**forceReport**](classOpenSkyhawk_1_1InputBase.md#function-forcereport) () = 0<br>_Read hardware state and emit a CAN EVT unconditionally._  |
|  [**InputBase**](classOpenSkyhawk_1_1InputBase.md) \* | [**next**](classOpenSkyhawk_1_1InputBase.md#function-next) () const<br>_Next input in the list; nullptr at end._  |
| virtual void | [**poll**](classOpenSkyhawk_1_1InputBase.md#function-poll) () = 0<br>_Read hardware state and emit a CAN EVT if state changed._  |
| virtual void | [**sampleTick**](classOpenSkyhawk_1_1InputBase.md#function-sampletick) () <br>_High-rate sample hook — called from a sampling ISR when the node runs one (e.g._ [_**ShiftBus**_](classOpenSkyhawk_1_1ShiftBus.md) _timer sampling, -DSHIFTBUS\_ISR\_HZ). Default no-op._ |




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


Emits **one** EVT\_ACTION frame on the press edge and nothing on release. The payload value is a selector, not a magnitude: 0 means `TOGGLE`, which [**PanelBridge**](namespacePanelBridge.md) renders as the DCS-BIOS action argument. The DCS-BIOS string never appears on this side of the bus.


**What it is for.** `Switch2Pos` sends absolute 0/1, so it needs a physical switch that latches. This class lets a _momentary_ part drive a control that latches _in the sim_: DCS-BIOS reads the control's current value, flips it, and writes it back, so each press toggles and the state persists. Because the sim supplies the current value, the panel cannot desync from it — unlike a physical latching switch, whose position can disagree with the sim after a cold start, a keyboard binding, or a mission script. The trade-off is that a momentary button shows no state at a glance.


**Only for controls that latch in the sim** — DCS-BIOS `defineToggleSwitch`. Pointing this at a `definePushButton` control is wrong: those are `momentary_last_position`, and a TOGGLE latches them on until the next press. Use `Switch2Pos` for those.


VALUE semantics (reverse = false, default): press (pin LOW — button closed, pulling pin to GND via board pull-up) — emit TOGGLE release (pin HIGH) — emit nothing


Holding the button emits exactly once: the press edge is consumed and the state does not change again until release, so there is no interval at which it could begin repeating.


Debounce: 20 ms of post-fire suppression, not a stability window. An action's first edge _is_ the event, so waiting for the level to settle would only add latency. Suppressing after the send costs nothing and stops contact chatter — which matters here more than for a level-tracking switch, because every TOGGLE flips sim state: an even number of bounces cancels and an odd number flips, so the final position would otherwise depend on bounce parity.


[**forceReport()**](classOpenSkyhawk_1_1ActionButton.md#function-forcereport) deliberately emits **nothing** — see its documentation. 


    
## Public Static Attributes Documentation




### variable DEBOUNCE\_MS 

```C++
constexpr uint32_t OpenSkyhawk::ActionButton::DEBOUNCE_MS;
```




<hr>
## Public Functions Documentation




### function ActionButton 

_Construct a momentary action button._ 
```C++
OpenSkyhawk::ActionButton::ActionButton (
    uint16_t controlId,
    PinRef pin,
    bool reverse=false
) 
```





**Parameters:**


* `controlId` DCSIN\_\* constant. Determines [**PanelBridge**](namespacePanelBridge.md) routing. 
* `pin` [**PinRef**](classPinRef.md) for the button input pin (GPIO, MCP23017, or [**ShiftBus**](classOpenSkyhawk_1_1ShiftBus.md) '165). 
* `reverse` false (default): active-LOW — board wiring holds HIGH, button pulls LOW. true: active-HIGH — board wiring holds LOW, button drives HIGH. [**configure()**](classOpenSkyhawk_1_1ActionButton.md#function-configure) does not enable internal pull-ups; the schematic must provide the required pull-up, pull-down, or active drive. 




        

<hr>



### function configure 

_Configure the input pin. Called by_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _after chip.begin()._
```C++
virtual void OpenSkyhawk::ActionButton::configure () override
```



Does not enable internal pull-ups; board wiring supplies the input bias.




**Note:**

Must not be called from the constructor — MCP23017 register writes require the chip to be initialised first. 





        
Implements [*OpenSkyhawk::InputBase::configure*](classOpenSkyhawk_1_1InputBase.md#function-configure)


<hr>



### function forceReport 

_Establish the current pin state as the baseline._ **Emits nothing.** __
```C++
virtual void OpenSkyhawk::ActionButton::forceReport () override
```



Called by [**PanelGroup**](namespacePanelGroup.md) during the boot EVT burst and on every SYNC\_REQ. Every other input class emits its current position here, which is idempotent for a switch — re-sending position 1 leaves the sim at 1. An action is not idempotent: emitting a TOGGLE would flip the sim switch on every resync, so this method is silent by design.


It is not a no-op, though. Seeding the baseline is what stops a button that is **physically held at boot** from reading as a press edge on the first [**poll()**](classOpenSkyhawk_1_1ActionButton.md#function-poll) and firing an unwanted TOGGLE. 


        
Implements [*OpenSkyhawk::InputBase::forceReport*](classOpenSkyhawk_1_1InputBase.md#function-forcereport)


<hr>



### function poll 

_Read the pin and emit one EVT\_ACTION on a debounced press edge._ 
```C++
virtual void OpenSkyhawk::ActionButton::poll () override
```



Called by [**PanelGroup::loop()**](namespacePanelGroup.md#function-loop) during normal operation. No-op until [**forceReport()**](classOpenSkyhawk_1_1ActionButton.md#function-forcereport) has been called at least once. 


        
Implements [*OpenSkyhawk::InputBase::poll*](classOpenSkyhawk_1_1InputBase.md#function-poll)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Inputs/ActionButton/ActionButton.h`

