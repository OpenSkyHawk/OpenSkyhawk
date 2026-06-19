

# Class OpenSkyhawk::NeedleGauge



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**NeedleGauge**](classOpenSkyhawk_1_1NeedleGauge.md)



_DCS-driven pointer gauge over any_ [_**MotorDriver**_](classOpenSkyhawk_1_1MotorDriver.md) _backend._

* `#include <NeedleGauge.h>`



Inherits the following classes: [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)






















































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**NeedleGauge**](#function-needlegauge) (uint16\_t controlId, uint16\_t mask, [**MotorDriver**](classOpenSkyhawk_1_1MotorDriver.md) & motor, const [**GaugeCal**](structOpenSkyhawk_1_1GaugeCal.md) & cal) <br>_Construct and register a pointer gauge._  |
| virtual void | [**configure**](#function-configure) () override<br>_Configure and home the motor (_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _)._ |
| virtual void | [**onControlPacket**](#function-oncontrolpacket) (uint16\_t controlId, uint16\_t value) override<br>_Retarget the motor from a CTRL\_BCAST packet. Stores only — never steps here._  |
| virtual void | [**update**](#function-update) () override<br>_Advance the motor toward its target (non-blocking)._  |


## Public Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
| virtual void | [**configure**](classOpenSkyhawk_1_1OutputBase.md#function-configure) () <br>_Configure hardware pins for this output._  |
|  [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) \* | [**next**](classOpenSkyhawk_1_1OutputBase.md#function-next) () const<br>_Next output in the list; nullptr at end._  |
| virtual void | [**onControlPacket**](classOpenSkyhawk_1_1OutputBase.md#function-oncontrolpacket) (uint16\_t controlId, uint16\_t value) = 0<br>_Called for every non-null ControlPacket in a CTRL\_BCAST frame._  |
| virtual void | [**update**](classOpenSkyhawk_1_1OutputBase.md#function-update) () <br>_Called every_ [_**PanelGroup::loop()**_](namespacePanelGroup.md#function-loop) _iteration._ |




## Public Static Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
|  [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) \* | [**head**](classOpenSkyhawk_1_1OutputBase.md#function-head) () <br>_Head of the self-registered linked list._  |












































## Protected Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md#function-outputbase) () <br>_Registers this instance into the linked list._  |






## Public Functions Documentation




### function NeedleGauge 

_Construct and register a pointer gauge._ 
```C++
OpenSkyhawk::NeedleGauge::NeedleGauge (
    uint16_t controlId,
    uint16_t mask,
    MotorDriver & motor,
    const GaugeCal & cal
) 
```





**Parameters:**


* `controlId` DCS-BIOS output address (A\_4E\_C\_\* from [**A4EC\_OutputIds.h**](A4EC__OutputIds_8h.md)). 
* `mask` Field mask (A\_4E\_C\_\*\_AM, or 0xFFFF for a whole-word gauge). 
* `motor` Caller-owned [**MotorDriver**](classOpenSkyhawk_1_1MotorDriver.md) (e.g. a [**StepperMotor**](classOpenSkyhawk_1_1StepperMotor.md)). Must outlive this. 
* `cal` Value→position calibration. Must outlive this. 




        

<hr>



### function configure 

_Configure and home the motor (_ [_**PanelGroup::setup()**_](namespacePanelGroup.md#function-setup) _)._
```C++
virtual void OpenSkyhawk::NeedleGauge::configure () override
```



Implements [*OpenSkyhawk::OutputBase::configure*](classOpenSkyhawk_1_1OutputBase.md#function-configure)


<hr>



### function onControlPacket 

_Retarget the motor from a CTRL\_BCAST packet. Stores only — never steps here._ 
```C++
virtual void OpenSkyhawk::NeedleGauge::onControlPacket (
    uint16_t controlId,
    uint16_t value
) override
```





**Parameters:**


* `controlId` Incoming packet controlId. Ignored if != \_controlId. 
* `value` Raw 16-bit DCS-BIOS value. 




        
Implements [*OpenSkyhawk::OutputBase::onControlPacket*](classOpenSkyhawk_1_1OutputBase.md#function-oncontrolpacket)


<hr>



### function update 

_Advance the motor toward its target (non-blocking)._ 
```C++
virtual void OpenSkyhawk::NeedleGauge::update () override
```



Implements [*OpenSkyhawk::OutputBase::update*](classOpenSkyhawk_1_1OutputBase.md#function-update)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Outputs/NeedleGauge/NeedleGauge.h`

