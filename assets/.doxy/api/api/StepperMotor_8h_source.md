

# File StepperMotor.h

[**File List**](files.md) **>** [**Drivers**](dir_da1b6a20235952b69490534d482f5898.md) **>** [**StepperMotor**](dir_f431add5022471a872df403ed217c535.md) **>** [**StepperMotor.h**](StepperMotor_8h.md)

[Go to the documentation of this file](StepperMotor_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>                       // PinRef
#include <Drivers/MotorDriver/MotorDriver.h>  // MotorDriver base

namespace OpenSkyhawk {

enum class HomeMode : uint8_t {
    STALL,   
    SENSOR   
    // ABSOLUTE — future: read an absolute encoder (AS5600); reports position, no seek.
};

enum class StepPattern : uint8_t {
    SWITEC_6STATE,  
    FULL_4STATE     
};

struct AccelPoint {
    uint16_t stepThreshold;  
    uint16_t delayUs;        
};

struct HomeSensor {
    bool     activeLow;     
    uint8_t  debounceMs;    
    uint16_t maxSeekSteps;  
};

struct StepperConfig {
    uint16_t          stepsPerRev;       
    StepPattern       pattern;           
    const AccelPoint* accel;             
    uint8_t           accelN;            
    HomeMode          home;              
    bool              homeSeekClockwise; 
    HomeSensor        sensor;            
    int16_t           homePosition;      
    int16_t           parkPosition;      
    int16_t           minPos;            
    int16_t           maxPos;            
    bool              wrap;              
    uint8_t           deadband;          
    bool              autoRecal;         
    uint32_t          recalDebounceMs;   
};

extern const AccelPoint kSwitecDefaultAccel[5];
constexpr uint8_t kSwitecDefaultAccelN = 5;

StepperConfig makeX27Config(int16_t homePosition, int16_t parkPosition,
                            int16_t minPos, int16_t maxPos,
                            HomeMode home = HomeMode::STALL,
                            bool homeSeekClockwise = false,
                            HomeSensor sensor = { true, 5, 2000 },
                            bool wrap = false, uint8_t deadband = 1,
                            bool autoRecal = false, uint32_t recalDebounceMs = 0,
                            uint16_t stepsPerRev = 720);

class StepperMotor : public MotorDriver {
public:
    StepperMotor(PinRef c1, PinRef c2, PinRef c3, PinRef c4, const StepperConfig& cfg,
                 PinRef homeSense = PinRef(), PinRef sleepEn = PinRef());

    void    configure() override;            
    void    home() override;                 
    void    moveTo(int32_t pos) override;    
    void    update() override;               
    int32_t position() const override;       

    bool homed() const { return _homed; }

#ifdef STEPPERMOTOR_TEST
    void    debugAdvance()            { advance(); }             
    int32_t debugCurrentStep() const  { return _currentStep; }
    int32_t debugTargetStep() const   { return _targetStep; }
    uint16_t debugVel() const         { return _vel; }
    uint16_t debugMicroDelay() const  { return _microDelay; }
    bool    debugStopped() const      { return _stopped; }
    bool    debugSensorAsserted() const { return sensorAsserted(false); }
    void    debugSetSensorOverride(int8_t level) { _sensorOverride = level; } 
#endif

private:
    // collaborators / config (plain // — EXTRACT_PRIVATE NO, not in API docs)
    PinRef       _coil[4];        // coil pins
    PinRef       _homeSense;      // home sensor (NC if unused)
    PinRef       _sleepEn;        // driver enable / ~SLEEP (NC if unused)
    StepperConfig _cfg;           // copied config (accel table referenced, not owned)
    uint16_t     _maxVel;         // last accel threshold = top speed gate

    // motion state (SwitecX25 model)
    int32_t      _currentStep;    // absolute step position
    int32_t      _targetStep;     // commanded target
    uint16_t     _vel;            // accel-steps proxy for velocity
    int8_t       _dir;            // +1 / -1 / 0
    bool         _stopped;        // true when settled at target
    uint8_t      _state;          // coil-state index (0..stateCount-1)
    uint16_t     _microDelay;     // delay until next step, µs
    uint32_t     _time0;          // micros() at last step
    bool         _homed;          // homing succeeded

    // auto-recal
    uint32_t     _lastRecalMs;    // millis() of last recal
    int8_t       _sensorOverride; // -1 = read pin; 0/1 = forced (test seam)

    // helpers
    void     writeIO();                    // energise coils for _state
    void     stepOnce(bool up);            // one detent in a direction
    void     advance();                    // SwitecX25 accel/step kernel
    bool     sensorAsserted(bool live) const; // single read through activeLow; live=true → bypass cache
    bool     sensorConfirmed() const;      // sensorAsserted stable for debounceMs
    bool     seekHomeBlocking();           // step toward sensor until confirmed or maxSeekSteps
    void     runToStopBlocking();          // advance() + delay until stopped (homing/park moves)
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
```


