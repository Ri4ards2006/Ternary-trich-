/*
 * Ternaryrator - High-Performance Ternary Logic Simulator
 * Target: Arduino Mega 2560 + ILI9341 TFT Touch Shield
 * Author: Senior Embedded Systems Engineer
 * Date: 2026-05-29
 * 
 * Memory Notes:
 * - Static allocation only. No heap usage after init.
 * - Trit registers use int8_t (signed byte) to represent {-1, 0, +1}.
 * - UI uses direct TFT primitives; no full-frame buffer (150KB would exhaust SRAM).
 * - Touch debouncing via temporal hysteresis (50ms) to prevent ISR-like spam.
 * 
 * Pin Mapping (Mega 2560 + Adafruit ILI9341 Touch Shield):
 * - TFT_CS:  10 (Hardware SPI CS)
 * - TFT_DC:   9 (Data/Command)
 * - TFT_RST:  8 (Reset, optional but recommended)
 * - TS_XP:    4 (Touch X+, shared logic with TFT control)
 * - TS_XM:   A2 (Touch X-)
 * - TS_YP:   A3 (Touch Y+)
 * - TS_YM:    9 (Touch Y-, shared with TFT_DC; shield multiplexes these)
 * - SPI MOSI: 51 (Hardware SPI)
 * - SPI MISO: 50 (Hardware SPI)
 * - SPI SCK:  52 (Hardware SPI)
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>

// -----------------------------------------------------------------------------
// Hardware Constants & Calibration
// -----------------------------------------------------------------------------
namespace HW {
    // TFT Pins
    static const uint8_t TFT_CS  = 10;
    static const uint8_t TFT_DC  = 9;
    static const uint8_t TFT_RST = 8;
    
    // Touch Pins (Shield-specific multiplexing)
    static const uint8_t TS_XP = 4;
    static const uint8_t TS_XM = A2;
    static const uint8_t TS_YP = A3;
    static const uint8_t TS_YM = 9; // Shares with TFT_DC; shield handles switching
    
    // Touch Calibration (raw ADC -> screen space)
    // Determined empirically for portrait orientation on Mega 2560
    static const int16_t TS_MINX = 150;
    static const int16_t TS_MAXX = 920;
    static const int16_t TS_MINY = 120;
    static const int16_t TS_MAXY = 900;
    
    // Timing
    static const unsigned long TOUCH_DEBOUNCE_MS = 50;
    static const unsigned long SCREEN_UPDATE_MS  = 33; // ~30 FPS cap
    static const unsigned long SIM_STEP_MS       = 500; // Simulation tick rate
}

// -----------------------------------------------------------------------------
// Color Palette: Retro-Scientific Dark Mode
// -----------------------------------------------------------------------------
namespace Colors {
    static const uint16_t BG        = 0x0000; // Black
    static const uint16_t PRIMARY     = 0x07FF; // Neon Cyan (0b0000011111111111)
    static const uint16_t SECONDARY = 0x041F; // Deep Blue-Cyan
    static const uint16_t ACCENT    = 0xF81F; // Magenta highlight
    static const uint16_t TEXT      = 0xFFFF; // White
    static const uint16_t DIM_TEXT  = 0x8410; // Gray
    static const uint16_t TRIT_N    = 0xF800; // Red for -1 (Negative)
    static const uint16_t TRIT_U    = 0xFFE0; // Yellow for 0 (Unknown)
    static const uint16_t TRIT_P    = 0x07E0; // Green for +1 (Positive)
    static const uint16_t NAV_BG    = 0x1082; // Dark slate
    static const uint16_t NAV_ACT   = 0x07FF; // Cyan active
    static const uint16_t HEADER_BG = 0x0004; // Near-black with blue tint
}

// -----------------------------------------------------------------------------
// Ternary Logic Core
// -----------------------------------------------------------------------------
class TernaryALU {
public:
    // Balanced Trit: -1 (N), 0 (U), +1 (P)
    enum Trit : int8_t { N = -1, U = 0, P = 1 };
    
    enum Operation : uint8_t {
        OP_MIN,     // Ternary AND / Min
        OP_MAX,     // Ternary OR / Max
        OP_INV,     // Ternary NOT / Inversion
        OP_ADD,     // Balanced Ternary Addition
        OP_XOR,     // Ternary XOR (mod 3 inequality)
        OP_CYCLE,   // Ternary Cycle (successor)
        OP_COUNT
    };
    
    TernaryALU() { clearRegisters(); }
    
    // Basic unary/binary operations
    static inline Trit inv(Trit a) {
        return static_cast<Trit>(-static_cast<int8_t>(a));
    }
    
    static inline Trit minOp(Trit a, Trit b) {
        return (a < b) ? a : b;
    }
    
    static inline Trit maxOp(Trit a, Trit b) {
        return (a > b) ? a : b;
    }
    
    // Balanced ternary addition with carry
    static Trit add(Trit a, Trit b, Trit& carry) {
        int8_t sum = static_cast<int8_t>(a) + static_cast<int8_t>(b) + static_cast<int8_t>(carry);
        if (sum > 1) { carry = P; sum -= 3; }
        else if (sum < -1) { carry = N; sum += 3; }
        else { carry = U; }
        return static_cast<Trit>(sum);
    }
    
    // Ternary XOR: true when inputs differ (mod 3 absolute difference > 0, normalized)
    static Trit xorOp(Trit a, Trit b) {
        int8_t d = abs(static_cast<int8_t>(a) - static_cast<int8_t>(b));
        if (d == 0) return U;
        return (d == 2) ? N : P; // diff 2 -> -1, diff 1 -> +1
    }
    
    // Ternary Cycle / Successor
    static Trit cycle(Trit a) {
        int8_t v = static_cast<int8_t>(a) + 1;
        if (v > 1) v = -1;
        return static_cast<Trit>(v);
    }
    
    // Execute selected operation on full registers
    void execute(Operation op) {
        Trit carry = U;
        for (uint8_t i = 0; i < 3; i++) {
            Trit a = regA[i];
            Trit b = regB[i];
            Trit r;
            switch (op) {
                case OP_MIN:   r = minOp(a, b); break;
                case OP_MAX:   r = maxOp(a, b); break;
                case OP_INV:   r = inv(a); break;
                case OP_ADD:   r = add(a, b, carry); break;
                case OP_XOR:   r = xorOp(a, b); break;
                case OP_CYCLE: r = cycle(a); break;
                default:       r = U; break;
            }
            regOut[i] = r;
        }
    }
    
    void clearRegisters() {
        for (uint8_t i = 0; i < 3; i++) {
            regA[i] = regB[i] = regOut[i] = U;
        }
    }
    
    void cycleRegister(uint8_t regIdx, uint8_t tritIdx) {
        Trit* reg = (regIdx == 0) ? regA : (regIdx == 1) ? regB : regOut;
        reg[tritIdx] = cycle(reg[tritIdx]);
    }
    
    Trit getReg(uint8_t regIdx, uint8_t tritIdx) const {
        if (regIdx == 0) return regA[tritIdx];
        if (regIdx == 1) return regB[tritIdx];
        return regOut[tritIdx];
    }
    
    void setReg(uint8_t regIdx, uint8_t tritIdx, Trit val) {
        if (regIdx == 0) regA[tritIdx] = val;
        else if (regIdx == 1) regB[tritIdx] = val;
        else regOut[tritIdx] = val;
    }
    
    Trit regA[3];
    Trit regB[3];
    Trit regOut[3];
};

// -----------------------------------------------------------------------------
// Touch Handler with Debouncing & Calibration
// -----------------------------------------------------------------------------
class TouchHandler {
public:
    TouchHandler() : ts(HW::TS_XP, HW::TS_YP, HW::TS_XM, HW::TS_YM, 300) {}
    
    void init() {
        // Ensure TFT pin is output before touch init to prevent bus contention
        pinMode(HW::TFT_DC, OUTPUT);
        _pressed = false;
        _tapped = false;
        _lastPressed = false;
        _lastDebounce = 0;
        _x = _y = 0;
    }
    
    // Non-blocking update. Call every loop iteration.
    void update() {
        _tapped = false;
        
        // Temporal debounce: ignore touch ADC sampling if within debounce window
        if (millis() - _lastDebounce < HW::TOUCH_DEBOUNCE_MS) {
            return;
        }
        
        TSPoint p = ts.getPoint();
        
        // Shield multiplexing: restore TFT pin mode immediately after touch read
        // because TS_YM shares TFT_DC on this shield.
        pinMode(HW::TS_XM, OUTPUT);
        pinMode(HW::TS_YP, OUTPUT);
        pinMode(HW::TS_XP, OUTPUT);
        digitalWrite(HW::TS_XP, LOW);
        digitalWrite(HW::TS_YP, HIGH);
        digitalWrite(HW::TS_XM, LOW);
        // Note: TFT library will reconfigure SPI on next draw call.
        
        if (p.z < 10) { // No significant pressure
            _lastPressed = _pressed;
            _pressed = false;
            return;
        }
        
        // Map raw touch to screen coordinates (portrait 240x320)
        // Empirical mapping: X and Y are often swapped on resistive overlays
        int16_t tx = map(p.y, HW::TS_MINY, HW::TS_MAXY, 0, 240);
        int16_t ty = map(p.x, HW::TS_MINX, HW::TS_MAXX, 0, 320);
        
        // Clamp
        tx = constrain(tx, 0, 239);
        ty = constrain(ty, 0, 319);
        
        _x = (uint16_t)tx;
        _y = (uint16_t)ty;
        
        if (!_lastPressed && _pressed) {
            // Already pressed, don't re-tap
        } else if (_lastPressed && !_pressed) {
            // Transition to pressed? No, this logic is inverted.
        }
        
        bool wasPressed = _lastPressed;
        _lastPressed = _pressed;
        _pressed = true;
        
        if (!wasPressed && _pressed) {
            _tapped = true;
            _lastDebounce = millis();
        }
    }
    
    bool isPressed() const { return _pressed; }
    bool isTapped() const { return _tapped; }
    uint16_t getX() const { return _x; }
    uint16_t getY() const { return _y; }
    
    // Determine if touch is in nav bar and which button (0-3)
    int8_t getNavRegion() const {
        if (_y < 270) return -1; // Not in nav area
        uint8_t btn = _x / 60;   // 4 buttons of 60px each
        if (btn > 3) btn = 3;
        return btn;
    }
    
private:
    TouchScreen ts;
    bool _pressed;
    bool _tapped;
    bool _lastPressed;
    uint16_t _x, _y;
    unsigned long _lastDebounce;
};

// -----------------------------------------------------------------------------
// UI Engine: Retro-Scientific Dashboard Renderer
// -----------------------------------------------------------------------------
class UIEngine {
public:
    UIEngine() : tft(HW::TFT_CS, HW::TFT_DC, HW::TFT_RST) {}
    
    void init() {
        tft.begin();
        tft.setRotation(0); // Portrait: 240x320
        tft.fillScreen(Colors::BG);
        tft.setTextWrap(false);
    }
    
    void clearScreen() {
        tft.fillScreen(Colors::BG);
    }
    
    void clearContent() {
        tft.fillRect(0, 30, 240, 240, Colors::BG);
    }
    
    // Persistent Header
    void drawHeader(const char* subtitle) {
        tft.fillRect(0, 0, 240, 30, Colors::HEADER_BG);
        tft.drawFastHLine(0, 30, 240, Colors::PRIMARY);
        
        tft.setCursor(4, 4);
        tft.setTextColor(Colors::PRIMARY);
        tft.setTextSize(2);
        tft.print("TERNARY");
        tft.setTextColor(Colors::TEXT);
        tft.print("RATOR");
        
        tft.setCursor(4, 20);
        tft.setTextColor(Colors::DIM_TEXT);
        tft.setTextSize(1);
        tft.print(subtitle);
        
        // Status indicator
        tft.fillRect(220, 8, 12, 12, Colors::TRIT_P);
        tft.drawRect(220, 8, 12, 12, Colors::TEXT);
    }
    
    // Navigation Bar (4 buttons)
    void drawNavBar(uint8_t activeIdx) {
        const char* labels[4] = {"ALU", "GATE", "SIM", "MENU"};
        tft.fillRect(0, 270, 240, 50, Colors::NAV_BG);
        tft.drawFastHLine(0, 270, 240, Colors::PRIMARY);
        
        for (uint8_t i = 0; i < 4; i++) {
            uint16_t x = i * 60;
            bool active = (i == activeIdx);
            uint16_t bg = active ? Colors::NAV_ACT : Colors::NAV_BG;
            uint16_t fg = active ? Colors::BG : Colors::TEXT;
            
            tft.fillRect(x + 1, 271, 58, 48, bg);
            tft.drawRect(x + 2, 272, 56, 46, active ? Colors::TEXT : Colors::DIM_TEXT);
            
            tft.setCursor(x + 14, 288);
            tft.setTextColor(fg);
            tft.setTextSize(2);
            tft.print(labels[i]);
        }
    }
    
    // Draw a single Trit as a neon cell
    void drawTritCell(uint16_t x, uint16_t y, uint16_t size, TernaryALU::Trit t, bool highlight) {
        uint16_t color;
        const char* label;
        switch (t) {
            case TernaryALU::N: color = Colors::TRIT_N; label = "N"; break;
            case TernaryALU::U: color = Colors::TRIT_U; label = "0"; break;
            case TernaryALU::P: color = Colors::TRIT_P; label = "P"; break;
        }
        
        uint16_t bg = highlight ? Colors::SECONDARY : Colors::BG;
        tft.fillRect(x, y, size, size, bg);
        tft.drawRect(x, y, size, size, color);
        if (highlight) {
            tft.drawRect(x+1, y+1, size-2, size-2, color);
        }
        
        tft.setCursor(x + size/2 - 6, y + size/2 - 8);
        tft.setTextColor(color);
        tft.setTextSize(2);
        tft.print(label);
    }
    
    // ALU Mode Visualizer
    void drawALU(const TernaryALU& alu, TernaryALU::Operation op) {
        clearContent();
        
        // Labels
        tft.setTextColor(Colors::PRIMARY);
        tft.setTextSize(1);
        tft.setCursor(10, 40); tft.print("REG A");
        tft.setCursor(10, 110); tft.print("REG B");
        tft.setCursor(10, 180); tft.print("OP");
        tft.setCursor(10, 250); tft.print("OUT");
        
        // Register A (y=55)
        for (uint8_t i = 0; i < 3; i++) {
            drawTritCell(60 + i * 55, 55, 45, alu.regA[i], false);
        }
        
        // Register B (y=125)
        for (uint8_t i = 0; i < 3; i++) {
            drawTritCell(60 + i * 55, 125, 45, alu.regB[i], false);
        }
        
        // Operation indicator (y=195)
        const char* opNames[] = {"MIN", "MAX", "INV", "ADD", "XOR", "CYC"};
        tft.setCursor(60, 195);
        tft.setTextColor(Colors::ACCENT);
        tft.setTextSize(2);
        tft.print(opNames[op]);
        tft.setTextColor(Colors::DIM_TEXT);
        tft.setCursor(130, 195);
        tft.setTextSize(1);
        tft.print("< tap A/B to cycle");
        
        // Output Register (y=235)
        for (uint8_t i = 0; i < 3; i++) {
            drawTritCell(60 + i * 55, 235, 45, alu.regOut[i], true);
        }
        
        // Decorative grid lines
        tft.drawFastHLine(10, 100, 220, Colors::DIM_TEXT);
        tft.drawFastHLine(10, 170, 220, Colors::DIM_TEXT);
        tft.drawFastHLine(10, 225, 220, Colors::DIM_TEXT);
    }
    
    // Gate Configuration / Truth Table
    void drawGateConfig(TernaryALU::Operation op) {
        clearContent();
        tft.setTextColor(Colors::PRIMARY);
        tft.setTextSize(1);
        tft.setCursor(10, 40); tft.print("TRUTH TABLE: ");
        
        const char* opNames[] = {"MIN", "MAX", "INV", "ADD", "XOR", "CYC"};
        tft.setTextColor(Colors::ACCENT);
        tft.setTextSize(2);
        tft.print(opNames[op]);
        
        // Draw 3x3 grid for binary ops, or 1x3 for unary
        uint8_t isUnary = (op == TernaryALU::OP_INV || op == TernaryALU::OP_CYCLE);
        
        TernaryALU::Trit inputs[3] = {TernaryALU::N, TernaryALU::U, TernaryALU::P};
        
        if (isUnary) {
            // Header
            tft.setTextColor(Colors::DIM_TEXT);
            tft.setCursor(40, 70); tft.print("IN");
            tft.setCursor(140, 70); tft.print("OUT");
            tft.drawFastHLine(20, 82, 200, Colors::DIM_TEXT);
            
            for (uint8_t i = 0; i < 3; i++) {
                uint8_t y = 90 + i * 50;
                TernaryALU::Trit in = inputs[i];
                TernaryALU::Trit out;
                if (op == TernaryALU::OP_INV) out = TernaryALU::inv(in);
                else out = TernaryALU::cycle(in);
                
                drawTritCell(30, y, 35, in, false);
                tft.setCursor(90, y + 12);
                tft.setTextColor(Colors::TEXT);
                tft.print("->");
                drawTritCell(130, y, 35, out, true);
            }
        } else {
            // Binary op grid
            tft.setTextColor(Colors::DIM_TEXT);
            tft.setCursor(20, 70); tft.print("A\\B");
            tft.setCursor(70, 70); tft.print("N");
            tft.setCursor(120, 70); tft.print("0");
            tft.setCursor(170, 70); tft.print("P");
            tft.drawFastHLine(15, 82, 210, Colors::DIM_TEXT);
            
            for (uint8_t a = 0; a < 3; a++) {
                uint8_t y = 90 + a * 55;
                tft.setTextColor(Colors::DIM_TEXT);
                tft.setCursor(25, y + 12);
                tft.print((a==0)?"N":(a==1)?"0":"P");
                
                for (uint8_t b = 0; b < 3; b++) {
                    TernaryALU::Trit out;
                    switch (op) {
                        case TernaryALU::OP_MIN: out = TernaryALU::minOp(inputs[a], inputs[b]); break;
                        case TernaryALU::OP_MAX: out = TernaryALU::maxOp(inputs[a], inputs[b]); break;
                        case TernaryALU::OP_ADD: {
                            TernaryALU::Trit c = TernaryALU::U;
                            out = TernaryALU::add(inputs[a], inputs[b], c);
                            break;
                        }
                        case TernaryALU::OP_XOR: out = TernaryALU::xorOp(inputs[a], inputs[b]); break;
                        default: out = TernaryALU::U; break;
                    }
                    drawTritCell(60 + b * 50, y, 40, out, false);
                }
            }
        }
    }
    
    // Simulation Mode: Animated Ternary Wave
    void drawSimulation(uint8_t step, uint8_t phase) {
        // Partial redraw optimization: only update wave region
        // Clear wave area
        tft.fillRect(0, 50, 240, 215, Colors::BG);
        
        tft.setTextColor(Colors::PRIMARY);
        tft.setTextSize(1);
        tft.setCursor(10, 40); tft.print("TERNARY COUNTER / WAVE");
        
        // Draw 3-phase ternary ring counter visualization
        const uint8_t baseY = 120;
        const uint8_t amp = 40;
        
        for (uint8_t i = 0; i < 20; i++) {
            uint8_t idx = (step + i) % 27; // 3^3 states
            int8_t val = (idx % 3) - 1; // -1, 0, +1 cycling
            
            uint16_t x = 10 + i * 11;
            int16_t y = baseY - (val * amp);
            
            uint16_t color = (val == -1) ? Colors::TRIT_N : (val == 0) ? Colors::TRIT_U : Colors::TRIT_P;
            
            tft.fillRect(x, y - 3, 8, 6, color);
            if (i < 19) {
                int8_t nextVal = ((idx + 1) % 3) - 1;
                int16_t y2 = baseY - (nextVal * amp);
                tft.drawLine(x + 4, y, x + 11, y2, Colors::DIM_TEXT);
            }
        }
        
        // Phase indicator
        tft.setCursor(10, 180);
        tft.setTextColor(Colors::TEXT);
        tft.setTextSize(1);
        tft.print("Phase: ");
        tft.print(phase);
        tft.print("  Step: ");
        tft.print(step);
        
        // Draw scrolling trit register
        uint8_t simReg[3];
        uint8_t s = step % 27;
        simReg[0] = (s / 9) % 3;
        simReg[1] = (s / 3) % 3;
        simReg[2] = s % 3;
        
        for (uint8_t i = 0; i < 3; i++) {
            TernaryALU::Trit t = static_cast<TernaryALU::Trit>(simReg[i] - 1);
            drawTritCell(60 + i * 55, 200, 45, t, true);
        }
    }
    
    // Menu screen
    void drawMenu() {
        clearContent();
        tft.setTextColor(Colors::PRIMARY);
        tft.setTextSize(2);
        tft.setCursor(30, 60);
        tft.print("TERNARYRATOR");
        
        tft.setTextColor(Colors::DIM_TEXT);
        tft.setTextSize(1);
        tft.setCursor(20, 90);
        tft.print("Balanced Ternary Logic Simulator");
        tft.setCursor(20, 105);
        tft.print("v1.0 | Mega 2560 | ILI9341");
        
        tft.setCursor(20, 140);
        tft.setTextColor(Colors::TEXT);
        tft.print("Select mode from nav bar below:");
        tft.setCursor(30, 160);
        tft.setTextColor(Colors::TRIT_P);
        tft.print("ALU: Interactive Arithmetic");
        tft.setCursor(30, 175);
        tft.setTextColor(Colors::TRIT_U);
        tft.print("GATE: Truth Table Explorer");
        tft.setCursor(30, 190);
        tft.setTextColor(Colors::TRIT_N);
        tft.print("SIM: Ternary Wave Animation");
    }
    
    // Feedback flash for touch
    void flashRegion(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
        tft.drawRect(x, y, w, h, Colors::ACCENT);
        delay(30); // Intentional short blocking for visual feedback only
        // Note: 30ms is acceptable for UX feedback; all other logic is non-blocking.
    }
    
private:
    Adafruit_ILI9341 tft;
};

// -----------------------------------------------------------------------------
// State Manager: Finite State Machine
// -----------------------------------------------------------------------------
class StateManager {
public:
    enum State : uint8_t {
        STATE_MENU = 0,
        STATE_ALU,
        STATE_GATE,
        STATE_SIM
    };
    
    StateManager() : _current(STATE_MENU), _previous(STATE_MENU), _dirty(true) {}
    
    void transitionTo(State s) {
        if (s == _current) return;
        _previous = _current;
        _current = s;
        _dirty = true;
        _transitionTime = millis();
    }
    
    State getState() const { return _current; }
    bool isDirty() { bool d = _dirty; _dirty = false; return d; }
    void markDirty() { _dirty = true; }
    unsigned long getTransitionTime() const { return _transitionTime; }
    
private:
    State _current;
    State _previous;
    bool _dirty;
    unsigned long _transitionTime;
};

// -----------------------------------------------------------------------------
// Global Instances
// -----------------------------------------------------------------------------
TernaryALU    alu;
UIEngine      ui;
TouchHandler  touch;
StateManager  state;

// FSM-local persistent data
TernaryALU::Operation currentOp = TernaryALU::OP_MIN;
uint8_t selectedReg = 0; // 0=A, 1=B
uint8_t simStep = 0;
uint8_t simPhase = 0;
unsigned long lastSimUpdate = 0;
unsigned long lastScreenUpdate = 0;

// -----------------------------------------------------------------------------
// Setup & Initialization
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    // Optional: wait for serial for debugging, but non-blocking after 2s
    unsigned long serialTimeout = millis();
    while (!Serial && (millis() - serialTimeout < 2000)) { /* non-blocking wait */ }
    
    // Initialize hardware
    ui.init();
    touch.init();
    
    // Splash screen
    ui.clearScreen();
    ui.drawHeader("SYSTEM BOOT");
    ui.drawMenu();
    ui.drawNavBar(3); // MENU active
    
    Serial.println(F("Ternaryrator initialized."));
    Serial.println(F("Mem check: Static allocation only."));
}

// -----------------------------------------------------------------------------
// Main Loop: Non-Blocking FSM
// -----------------------------------------------------------------------------
void loop() {
    unsigned long now = millis();
    
    // 1. Input Sampling (always active)
    touch.update();
    
    // 2. Global Navigation Handling (top priority)
    if (touch.isTapped()) {
        int8_t navBtn = touch.getNavRegion();
        if (navBtn >= 0) {
            // Map nav button to state
            StateManager::State targetState;
            switch (navBtn) {
                case 0: targetState = StateManager::STATE_ALU; break;
                case 1: targetState = StateManager::STATE_GATE; break;
                case 2: targetState = StateManager::STATE_SIM; break;
                case 3: targetState = StateManager::STATE_MENU; break;
                default: targetState = StateManager::STATE_MENU; break;
            }
            if (targetState != state.getState()) {
                state.transitionTo(targetState);
                // Immediate audio/visual feedback could go here
            }
        }
    }
    
    // 3. State-Specific Logic & Interaction
    handleStateLogic(now);
    
    // 4. Rendering (throttled to ~30 FPS to prevent SPI bus saturation)
    if (now - lastScreenUpdate >= HW::SCREEN_UPDATE_MS) {
        lastScreenUpdate = now;
        renderState();
    }
}

// -----------------------------------------------------------------------------
// State Logic Handlers
// -----------------------------------------------------------------------------
void handleStateLogic(unsigned long now) {
    StateManager::State s = state.getState();
    
    switch (s) {
        case StateManager::STATE_ALU:
            handleALULogic();
            break;
        case StateManager::STATE_GATE:
            handleGateLogic();
            break;
        case StateManager::STATE_SIM:
            handleSimLogic(now);
            break;
        case StateManager::STATE_MENU:
        default:
            // Menu is static; no interaction beyond nav
            break;
    }
}

void handleALULogic() {
    if (!touch.isTapped()) return;
    
    uint16_t tx = touch.getX();
    uint16_t ty = touch.getY();
    
    // Check for nav (already handled), ignore if in nav
    if (ty >= 270) return;
    
    // Check register A trits (y=55, size=45, gap=55, start=60)
    if (ty >= 55 && ty <= 100) {
        for (uint8_t i = 0; i < 3; i++) {
            if (tx >= 60 + i * 55 && tx <= 105 + i * 55) {
                alu.cycleRegister(0, i); // Cycle A[i]
                alu.execute(currentOp);
                state.markDirty();
                return;
            }
        }
    }
    
    // Check register B trits (y=125)
    if (ty >= 125 && ty <= 170) {
        for (uint8_t i = 0; i < 3; i++) {
            if (tx >= 60 + i * 55 && tx <= 105 + i * 55) {
                alu.cycleRegister(1, i); // Cycle B[i]
                alu.execute(currentOp);
                state.markDirty();
                return;
            }
        }
    }
    
    // Check operation selector (y=195 area)
    if (ty >= 185 && ty <= 220) {
        currentOp = static_cast<TernaryALU::Operation>((currentOp + 1) % TernaryALU::OP_COUNT);
        alu.execute(currentOp);
        state.markDirty();
    }
}

void handleGateLogic() {
    if (!touch.isTapped()) return;
    uint16_t ty = touch.getY();
    if (ty >= 270) return; // Nav only
    
    // Cycle through operations on any content tap
    currentOp = static_cast<TernaryALU::Operation>((currentOp + 1) % TernaryALU::OP_COUNT);
    state.markDirty();
}

void handleSimLogic(unsigned long now) {
    if (now - lastSimUpdate >= HW::SIM_STEP_MS) {
        lastSimUpdate = now;
        simStep++;
        if (simStep >= 27) {
            simStep = 0;
            simPhase++;
        }
        state.markDirty();
    }
}

// -----------------------------------------------------------------------------
// Render Dispatcher
// -----------------------------------------------------------------------------
void renderState() {
    if (!state.isDirty()) return;
    
    StateManager::State s = state.getState();
    
    // Always redraw header and nav on state change
    ui.drawHeader(getStateTitle(s));
    ui.drawNavBar(static_cast<uint8_t>(s));
    
    switch (s) {
        case StateManager::STATE_ALU:
            ui.drawALU(alu, currentOp);
            break;
        case StateManager::STATE_GATE:
            ui.drawGateConfig(currentOp);
            break;
        case StateManager::STATE_SIM:
            ui.drawSimulation(simStep, simPhase);
            break;
        case StateManager::STATE_MENU:
            ui.drawMenu();
            break;
    }
}

const char* getStateTitle(StateManager::State s) {
    switch (s) {
        case StateManager::STATE_ALU:  return "ALU MODE";
        case StateManager::STATE_GATE: return "GATE CONFIG";
        case StateManager::STATE_SIM:  return "SIMULATION";
        default: return "MAIN MENU";
    }
}