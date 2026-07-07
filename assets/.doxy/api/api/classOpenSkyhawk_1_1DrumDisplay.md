

# Class OpenSkyhawk::DrumDisplay



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**DrumDisplay**](classOpenSkyhawk_1_1DrumDisplay.md)



_Rolling-drum OLED readout. One instance == one OLED panel._ [More...](#detailed-description)

* `#include <DrumDisplay.h>`



Inherits the following classes: [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md),  [OpenSkyhawk::I2cHealth](classOpenSkyhawk_1_1I2cHealth.md),  [OpenSkyhawk::FaultSource](classOpenSkyhawk_1_1FaultSource.md)














## Public Types

| Type | Name |
| ---: | :--- |
| enum uint8\_t | [**Fault**](#enum-fault)  <br>_Which I2C hop failed the last reachability probe (feeds node health reporting, #163)._  |
















































































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**DrumDisplay**](#function-drumdisplay-12) (U8G2 & oled, const [**DrumReadout**](structOpenSkyhawk_1_1DrumReadout.md) & readout, [**DrumFont**](namespaceOpenSkyhawk.md#enum-drumfont) font=DrumFont::LARGE, float xOffsetMm=0.0f, float yOffsetMm=0.0f) <br>_Construct and register a direct-bus drum display._  |
|   | [**DrumDisplay**](#function-drumdisplay-22) (U8G2 & oled, const [**DrumReadout**](structOpenSkyhawk_1_1DrumReadout.md) & readout, [**I2cMux**](classOpenSkyhawk_1_1I2cMux.md) & mux, uint8\_t channel, [**DrumFont**](namespaceOpenSkyhawk.md#enum-drumfont) font=DrumFont::LARGE, float xOffsetMm=0.0f, float yOffsetMm=0.0f) <br>_Construct and register a muxed drum display (one TCA9548A branch)._  |
| virtual void | [**configure**](#function-configure) () override<br>_Compute pixel geometry from the panel + descriptor, set the font, blank the panel._  |
| virtual [**NodeFaultCode**](NodeStatus_8h.md#enum-nodefaultcode) | [**faultCode**](#function-faultcode) () override const<br>[_**FaultSource**_](classOpenSkyhawk_1_1FaultSource.md) _: I2C\_PERIPHERAL when the_[_**I2cHealth**_](classOpenSkyhawk_1_1I2cHealth.md) _breaker is tripped, else NONE (#163). Cached breaker state only — no I2C op. The node aggregator packs this into HEALTH\_n.faultId._ |
| virtual const char \* | [**faultDetail**](#function-faultdetail) () override const<br>_DiagSerial-only fault detail (#163): which I2C hop failed the last probe._  |
| virtual void | [**onControlPacket**](#function-oncontrolpacket) (uint16\_t controlId, uint16\_t value) override<br>_Decode one CTRL\_BCAST packet into this readout's digits/flag. Never draws._  |
|  void | [**setFontSize**](#function-setfontsize) ([**DrumFont**](namespaceOpenSkyhawk.md#enum-drumfont) font) <br>_Change glyph size at runtime (e.g. swap a cramped 6-digit readout to SMALL)._  |
|  void | [**setOffset**](#function-setoffset) (float xOffsetMm, float yOffsetMm) <br>_Re-register the digit block to the faceplate window at runtime._  |
| virtual void | [**update**](#function-update) () override<br>_Advance the ease/snap animation and push one frame if needed._  |


## Public Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
| virtual void | [**configure**](classOpenSkyhawk_1_1OutputBase.md#function-configure) () <br>_Configure hardware pins for this output._  |
|  [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) \* | [**next**](classOpenSkyhawk_1_1OutputBase.md#function-next) () const<br>_Next output in the list; nullptr at end._  |
| virtual void | [**onControlPacket**](classOpenSkyhawk_1_1OutputBase.md#function-oncontrolpacket) (uint16\_t controlId, uint16\_t value) = 0<br>_Called for every non-null ControlPacket in a CTRL\_BCAST frame._  |
| virtual void | [**update**](classOpenSkyhawk_1_1OutputBase.md#function-update) () <br>_Called every_ [_**PanelGroup::loop()**_](namespacePanelGroup.md#function-loop) _iteration._ |


## Public Functions inherited from OpenSkyhawk::I2cHealth

See [OpenSkyhawk::I2cHealth](classOpenSkyhawk_1_1I2cHealth.md)

| Type | Name |
| ---: | :--- |
|  bool | [**i2cHealthy**](classOpenSkyhawk_1_1I2cHealth.md#function-i2chealthy) () const<br>_Breaker state — true while the device last probed reachable._  |


## Public Functions inherited from OpenSkyhawk::FaultSource

See [OpenSkyhawk::FaultSource](classOpenSkyhawk_1_1FaultSource.md)

| Type | Name |
| ---: | :--- |
| virtual [**NodeFaultCode**](NodeStatus_8h.md#enum-nodefaultcode) | [**faultCode**](classOpenSkyhawk_1_1FaultSource.md#function-faultcode) () const<br>_Current fault code (_ NodeFaultCode::NONE _when healthy). Cheap/const, cached state only._ |
| virtual const char \* | [**faultDetail**](classOpenSkyhawk_1_1FaultSource.md#function-faultdetail) () const<br>_Human-readable fault detail for the local DiagSerial tap only — never on the wire._  |
|  [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md) \* | [**next**](classOpenSkyhawk_1_1FaultSource.md#function-next) () const<br>_Next fault source; nullptr at end._  |




## Public Static Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
|  [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) \* | [**head**](classOpenSkyhawk_1_1OutputBase.md#function-head) () <br>_Head of the self-registered linked list._  |




## Public Static Functions inherited from OpenSkyhawk::FaultSource

See [OpenSkyhawk::FaultSource](classOpenSkyhawk_1_1FaultSource.md)

| Type | Name |
| ---: | :--- |
|  [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md) \* | [**head**](classOpenSkyhawk_1_1FaultSource.md#function-head) () <br>_Head of the self-registered fault-source list._  |






























## Protected Static Attributes inherited from OpenSkyhawk::I2cHealth

See [OpenSkyhawk::I2cHealth](classOpenSkyhawk_1_1I2cHealth.md)

| Type | Name |
| ---: | :--- |
|  constexpr uint32\_t | [**I2C\_RETRY\_MS**](classOpenSkyhawk_1_1I2cHealth.md#variable-i2c_retry_ms)   = `2000`<br>_Back-off between retries once tripped (ms). A couple of seconds keeps the bus quiet._  |




















































## Protected Functions

| Type | Name |
| ---: | :--- |
| virtual bool | [**i2cProbe**](#function-i2cprobe) () override<br>_Contract: probe this device's reachability (e.g. the mux ACKs_ _and_ _the device ACKs)._ |


## Protected Functions inherited from OpenSkyhawk::OutputBase

See [OpenSkyhawk::OutputBase](classOpenSkyhawk_1_1OutputBase.md)

| Type | Name |
| ---: | :--- |
|   | [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md#function-outputbase) () <br>_Registers this instance into the linked list._  |


## Protected Functions inherited from OpenSkyhawk::I2cHealth

See [OpenSkyhawk::I2cHealth](classOpenSkyhawk_1_1I2cHealth.md)

| Type | Name |
| ---: | :--- |
| virtual bool | [**i2cProbe**](classOpenSkyhawk_1_1I2cHealth.md#function-i2cprobe) () = 0<br>_Contract: probe this device's reachability (e.g. the mux ACKs_ _and_ _the device ACKs)._ |
|  bool | [**i2cReachable**](classOpenSkyhawk_1_1I2cHealth.md#function-i2creachable) () <br>_Gate for every I2C op. Rate-limits the probe while tripped; trips/heals on the result._  |
|   | [**~I2cHealth**](classOpenSkyhawk_1_1I2cHealth.md#function-i2chealth) () = default<br> |


## Protected Functions inherited from OpenSkyhawk::FaultSource

See [OpenSkyhawk::FaultSource](classOpenSkyhawk_1_1FaultSource.md)

| Type | Name |
| ---: | :--- |
|   | [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md#function-faultsource) () <br>_Registers this instance into the list._  |
|   | [**~FaultSource**](classOpenSkyhawk_1_1FaultSource.md#function-faultsource) () = default<br>_Protected, non-virtual: a base/mixin, never deleted through this type._  |










## Detailed Description


Construct at global scope so [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) self-registers it. [**onControlPacket()**](classOpenSkyhawk_1_1DrumDisplay.md#function-oncontrolpacket) only decodes + flags dirty (cheap); [**update()**](classOpenSkyhawk_1_1DrumDisplay.md#function-update) does the ~60 fps gate, channel reselect, ease+snap, render, and the single expensive sendBuffer(). 


    
## Public Types Documentation




### enum Fault 

_Which I2C hop failed the last reachability probe (feeds node health reporting, #163)._ 
```C++
enum OpenSkyhawk::DrumDisplay::Fault {
    None,
    Mux,
    Device
};
```




<hr>
## Public Functions Documentation




### function DrumDisplay [1/2]

_Construct and register a direct-bus drum display._ 
```C++
OpenSkyhawk::DrumDisplay::DrumDisplay (
    U8G2 & oled,
    const DrumReadout & readout,
    DrumFont font=DrumFont::LARGE,
    float xOffsetMm=0.0f,
    float yOffsetMm=0.0f
) 
```





**Parameters:**


* `oled` Caller-owned U8G2 (already begin()'d, rotation set). Must outlive this. 
* `readout` Descriptor for this readout (sources, geometry, flag). Must outlive this. 
* `font` Per-mounting glyph size. Default DrumFont::LARGE. 
* `xOffsetMm` X shift (mm) of the whole digit block, registers it to the faceplate window. 
* `yOffsetMm` Y shift (mm) of the digit block centre line. 



**Note:**

The sketch owns Wire.begin() + oled.begin(). Geometry is auto-fitted in [**configure()**](classOpenSkyhawk_1_1DrumDisplay.md#function-configure). 





        

<hr>



### function DrumDisplay [2/2]

_Construct and register a muxed drum display (one TCA9548A branch)._ 
```C++
OpenSkyhawk::DrumDisplay::DrumDisplay (
    U8G2 & oled,
    const DrumReadout & readout,
    I2cMux & mux,
    uint8_t channel,
    DrumFont font=DrumFont::LARGE,
    float xOffsetMm=0.0f,
    float yOffsetMm=0.0f
) 
```





**Parameters:**


* `oled` Caller-owned U8G2. Must outlive this. 
* `readout` Descriptor. Must outlive this. 
* `mux` Shared TCA9548A selector. Must outlive this. 
* `channel` TCA9548A channel 0–7 this panel sits on. 
* `font` Per-mounting glyph size. Default DrumFont::LARGE. 
* `xOffsetMm` X registration shift (mm). 
* `yOffsetMm` Y registration shift (mm). 



**Note:**

The class calls mux.select(channel) before geometry fit in [**configure()**](classOpenSkyhawk_1_1DrumDisplay.md#function-configure) and before each sendBuffer() so interleaved displays never write to the wrong panel. 





        

<hr>



### function configure 

_Compute pixel geometry from the panel + descriptor, set the font, blank the panel._ 
```C++
virtual void OpenSkyhawk::DrumDisplay::configure () override
```





**Note:**

Called once by [**PanelGroup::setup()**](namespacePanelGroup.md#function-setup) after chip init. Selects the mux channel (if any) first, reads getDisplayWidth()/Height(), runs the geometry fit, and clears the OLED. The sketch owns oled.begin(); this does NOT call begin(). 





        
Implements [*OpenSkyhawk::OutputBase::configure*](classOpenSkyhawk_1_1OutputBase.md#function-configure)


<hr>



### function faultCode 

[_**FaultSource**_](classOpenSkyhawk_1_1FaultSource.md) _: I2C\_PERIPHERAL when the_[_**I2cHealth**_](classOpenSkyhawk_1_1I2cHealth.md) _breaker is tripped, else NONE (#163). Cached breaker state only — no I2C op. The node aggregator packs this into HEALTH\_n.faultId._
```C++
inline virtual NodeFaultCode OpenSkyhawk::DrumDisplay::faultCode () override const
```



Implements [*OpenSkyhawk::FaultSource::faultCode*](classOpenSkyhawk_1_1FaultSource.md#function-faultcode)


<hr>



### function faultDetail 

_DiagSerial-only fault detail (#163): which I2C hop failed the last probe._ 
```C++
inline virtual const char * OpenSkyhawk::DrumDisplay::faultDetail () override const
```



Implements [*OpenSkyhawk::FaultSource::faultDetail*](classOpenSkyhawk_1_1FaultSource.md#function-faultdetail)


<hr>



### function onControlPacket 

_Decode one CTRL\_BCAST packet into this readout's digits/flag. Never draws._ 
```C++
virtual void OpenSkyhawk::DrumDisplay::onControlPacket (
    uint16_t controlId,
    uint16_t value
) override
```





**Parameters:**


* `controlId` DCS-BIOS output address from the packet. Ignored unless it matches a [**DrumSource.address**](structOpenSkyhawk_1_1DrumSource.md#variable-address) or the flag.address of this readout. 
* `value` 16-bit DCS-BIOS value (0..65535) for that address. 



**Note:**

Cheap: decodes value→digit(s), splices into the target number, sets the dirty flag. The expensive full-buffer I2C send happens in [**update()**](classOpenSkyhawk_1_1DrumDisplay.md#function-update), never here. 





        
Implements [*OpenSkyhawk::OutputBase::onControlPacket*](classOpenSkyhawk_1_1OutputBase.md#function-oncontrolpacket)


<hr>



### function setFontSize 

_Change glyph size at runtime (e.g. swap a cramped 6-digit readout to SMALL)._ 
```C++
void OpenSkyhawk::DrumDisplay::setFontSize (
    DrumFont font
) 
```





**Parameters:**


* `font` New DrumFont. Re-fits geometry on the next [**update()**](classOpenSkyhawk_1_1DrumDisplay.md#function-update) so cells re-auto-fit. 




        

<hr>



### function setOffset 

_Re-register the digit block to the faceplate window at runtime._ 
```C++
void OpenSkyhawk::DrumDisplay::setOffset (
    float xOffsetMm,
    float yOffsetMm
) 
```





**Parameters:**


* `xOffsetMm` New X shift (mm) of the digit block — measure the cutout misalignment after assembly. 
* `yOffsetMm` New Y shift (mm, centre line) of the digit block. 



**Note:**

Marks geometry dirty; the new offsets apply on the next rendered frame. Resolution- independent (converted to px via the panel scale), so the same mm value registers correctly on any OLED size. 




**Note:**

Rounds to whole pixels, so the smallest useful step is ~0.25 mm (≈1 px); a 0.1 mm nudge is sub-pixel and may not move. The 1.3" 128x64 (~0.23 mm/px) is the coarser panel. 





        

<hr>



### function update 

_Advance the ease/snap animation and push one frame if needed._ 
```C++
virtual void OpenSkyhawk::DrumDisplay::update () override
```





**Note:**

Called every loop(). ~60 fps frame gate; early-out when idle (settled AND not dirty). On a live frame: mux.select(channel), ease each place toward target/10^place with SNAP\_SETTLE jump handling, render every cell with setClipWindow, then one sendBuffer(). 





        
Implements [*OpenSkyhawk::OutputBase::update*](classOpenSkyhawk_1_1OutputBase.md#function-update)


<hr>
## Protected Functions Documentation




### function i2cProbe 

_Contract: probe this device's reachability (e.g. the mux ACKs_ _and_ _the device ACKs)._
```C++
virtual bool OpenSkyhawk::DrumDisplay::i2cProbe () override
```





**Returns:**

true if reachable. The implementer records any fault detail it wants to report. 




**Note:**

Must be cheap — a single address probe, no payload — and must not throw or block beyond one bounded I2C transaction. 





        
Implements [*OpenSkyhawk::I2cHealth::i2cProbe*](classOpenSkyhawk_1_1I2cHealth.md#function-i2cprobe)


<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/DrumDisplay/DrumDisplay.h`

