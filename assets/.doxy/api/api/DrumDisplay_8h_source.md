

# File DrumDisplay.h

[**File List**](files.md) **>** [**DrumDisplay**](dir_b8ac8c4bf654b399fff34fe5003d7d6c.md) **>** [**DrumDisplay.h**](DrumDisplay_8h.md)

[Go to the documentation of this file](DrumDisplay_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>      // OutputBase
#include <U8g2lib.h>
#include <Helpers/I2cMux/I2cMux.h>
#include <Helpers/I2cHealth/I2cHealth.h>

namespace OpenSkyhawk {

// ── Per-mounting font selection (ctor param, runtime-writable) ────────────────

enum class DrumFont : uint8_t {
    SMALL = 0,  
    LARGE = 1,  
};

enum class DrumScroll : uint8_t {
    EASE_ONLY   = 0,  
    SNAP_SETTLE = 1,  
};

// ── Descriptor structs (mm-based, resolution-independent) ─────────────────────

struct DrumSource {
    uint16_t address;   
    uint16_t mask;      
    uint8_t  nDigits;   
    uint8_t  place;     
};

struct DrumGlyph {
    char    ch;         
    uint8_t afterCol;   
    float   widthMm;    
};

struct DrumFlag {
    bool        enabled;      
    uint16_t    address;      
    uint16_t    mask;         
    const char* faces;        
    uint8_t     atVisualCol;  
    float       widthMm;      
};

struct DrumReadout {
    const DrumSource* sources;        
    uint8_t  nSources;                
    uint8_t  nDigits;                 
    float    digitWidthMm;            
    float    digitHeightMm;           
    float    interDigitGapMm;         
    float    groupGapMm;              
    uint8_t  groupSize;               
    const DrumGlyph* glyphs;          
    uint8_t  nGlyphs;                 
    DrumFlag flag;                    
    DrumScroll scroll;                
    float    snapThreshold;           
};

// ── The output class ──────────────────────────────────────────────────────────

class DrumDisplay : public OutputBase, public I2cHealth {
public:
    enum class Fault : uint8_t { None, Mux, Device };

    DrumDisplay(U8G2& oled, const DrumReadout& readout,
                DrumFont font = DrumFont::LARGE,
                float xOffsetMm = 0.0f, float yOffsetMm = 0.0f);

    DrumDisplay(U8G2& oled, const DrumReadout& readout,
                I2cMux& mux, uint8_t channel,
                DrumFont font = DrumFont::LARGE,
                float xOffsetMm = 0.0f, float yOffsetMm = 0.0f);

    void configure() override;

    void onControlPacket(uint16_t controlId, uint16_t value) override;

    void update() override;

    void setFontSize(DrumFont font);

    void setOffset(float xOffsetMm, float yOffsetMm);

#ifdef DRUMDISPLAY_TEST
    long    debugTarget() const     { return _target; }      
    long    debugFlagTarget() const { return _flagTarget; }  
    uint8_t debugCellCount() const  { return _nCells; }      
    int16_t debugRowWidth() const;                           
    int16_t debugCellX0() const     { return _nCells ? _cellX[0] : 0; }  
    bool     debugHealthy() const     { return i2cHealthy(); }                  
    uint8_t  debugFault() const       { return static_cast<uint8_t>(_fault); }  
    uint32_t debugRenderCount() const { return _renderCount; }                  
    void     debugForceProbe(int v)   { _probeOverride = v; }                   
    bool     debugReachable()          { return i2cReachable(); }               
    uint32_t debugProbeCount() const   { return _probeCount; }                  
#endif

protected:
    bool i2cProbe() override;  // I2cHealth contract: mux present + OLED ACKs on its channel; sets _fault

private:
    static constexpr uint8_t MAX_CELLS = 8;  // 6 digits + 1 glyph + 1 flag

    uint8_t oledAddr() const;        // OLED 7-bit address, read from the U8G2 object (for the probe)
    Fault   _fault = Fault::None;    // which hop failed the last probe (mux vs device)
#ifdef DRUMDISPLAY_TEST
    uint32_t _renderCount  = 0;      // sendBuffer() calls — render-skip assertion
    uint32_t _probeCount   = 0;      // i2cProbe() calls — back-off assertion
    int      _probeOverride = -1;    // -1 = real probe, 0 = force fail, 1 = force ok
#endif

    // collaborators / config (plain // — EXTRACT_PRIVATE NO, not rendered in API docs)
    U8G2*              _oled;        // caller-owned panel
    const DrumReadout* _r;           // descriptor (not owned)
    I2cMux*            _mux;          // nullptr for direct-bus instances
    uint8_t            _channel;      // mux channel; ignored when _mux == nullptr
    DrumFont           _font;         // current glyph size
    float              _xOffMm, _yOffMm;  // registration offset, mm (→ px via PX_PER_MM in fitGeometry)

    // decoded value state (set in onControlPacket, read in update)
    long               _target;       // combined integer the digit row should show
    long               _flagTarget;   // flag face index target (0..nFaces-1)
    volatile bool      _dirty;        // a source/flag changed since last render
    bool               _hasState;     // false until first matching packet (blank until then)

    // animation state (continuous tape positions, [0] = least-significant)
    float              _pos[6];       // up to 6 digit tapes; _r->nDigits used
    float              _flagPos;      // flag tape continuous position

    // geometry, auto-fitted in fitGeometry()
    bool               _geomDirty;    // recompute on next frame (font/offset changed)
    int16_t            _colW;         // digit cell width, px
    int16_t            _cellH;        // roll-window height, px
    int16_t            _gap;          // inter-cell gap, px
    int16_t            _flagW;        // flag cell width, px (0 if no flag)
    int16_t            _cy;           // row centre Y, px (display centre + yOff)
    int16_t            _cellX[MAX_CELLS];     // precomputed left-X of each visual cell, px
    int16_t            _cellW[MAX_CELLS];     // width of each visual cell, px
    uint8_t            _cellKind[MAX_CELLS];  // 0 = digit, 1 = glyph, 2 = flag
    int16_t            _cellData[MAX_CELLS];  // digit: place index; glyph: index into glyphs[]
    uint8_t            _nCells;       // total visual cells (digits + glyphs + flag)
    uint32_t           _lastFrameMs;  // ~60 fps gate timestamp

    // helpers
    void fitGeometry();                              // mm + font + offset → px, fill _cellX[]
    void drawTape(int16_t cx, float p, int16_t w);   // ported rolling digit tape
    void drawFlag(int16_t cx, float p, int16_t w);   // ported 2-face flag tape
    bool settled() const;                            // every |target/10^k − pos[k]| < epsilon
    const uint8_t* fontPtr() const;                  // ProFont face for _font
    static long decodeDigits(uint16_t value, uint16_t mask, uint8_t nDigits);
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


