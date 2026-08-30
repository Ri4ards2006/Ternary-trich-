/*
 * =============================================================================
 * TERNARYRATOR — High-Performance Ternary Logic Simulator
 * Target:    Arduino Mega 2560 + MCUFRIEND 2.4" TFT Touch Shield (ILI9341)
 * Author:    Senior Embedded Systems Engineer
 * Date:      2026-05-29
 * License:   Proprietary / Educational Reference
 * =============================================================================
 *
 * MEMORY & EFFICIENCY NOTES:
 * - 100 % static allocation. Zero heap fragmentation after setup().
 * - Trit registers use int8_t (signed byte) to encode {-1, 0, +1}.
 * - UI renders via direct TFT primitives; no full-frame buffer (150 KB would
 *   exhaust the Mega 2560's 8 KB SRAM).
 * - Non-blocking temporal model: millis() deltas drive all periodic events.
 *   Only one 30 ms blocking call exists for deliberate touch-flash UX feedback.
 *
 * PIN MAPPING (Mega 2560 + MCUFRIEND 2.4" TFT Touch Shield):
 *   The MCUFRIEND shield uses a parallel 8/16-bit interface. Control and data
 *   pins are hard-wired to the Mega header; the MCUFRIEND_kbv library manages
 *   them internally. Only the TouchScreen overlay pins are user-visible below.
 *
 *   TFT Parallel Bus (handled by library):
 *     LCD_RD  -> A0
 *     LCD_WR  -> A1
 *     LCD_RS  -> A2   (shared with Touch XM; restored after every sample)
 *     LCD_CS  -> A3
 *     LCD_RST -> A4
 *     LCD_D0  -> D8   ... D7 -> D13 (8-bit mode)
 *
 *   Touch Overlay (resistive, 4-wire):
 *     TS_XP   -> D6   (X+, also called Left)
 *     TS_XM   -> A2   (X-, also called Right,  shares LCD_RS)
 *     TS_YP   -> A1   (Y+, also called Bottom, shares LCD_WR)
 *     TS_YM   -> D7   (Y-, also called Top)
 *
 *   SPI is NOT used for the TFT in this configuration; the shield occupies
 *   D8-D13 and A0-A4. If other SPI devices are present, use a separate CS.
 * =============================================================================
 */

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

// -----------------------------------------------------------------------------
// Hardware Constants & Calibration
// -----------------------------------------------------------------------------
namespace HW {
    /* Touch overlay ADC thresholds and screen-space calibration.
     * These values are empirically determined for portrait orientation
     * (240 x 320) on the Mega 2560 with the MCUFRIEND shield. */
    static const uint8_t  TS_XP = 6;
    static const uint8_t  TS_XM = A2;
    static const uint8_t  TS_YP = A1;
    static const uint8_t  TS_YM = 7;

    /* Resistive touch pressure reference. 300 Ω is a typical baseline
     * for the MCUFRIEND 2.4" shield; lower values yield higher sensitivity. */
    static const uint16_t TS_OHMS = 300;

    /* Raw ADC bounds -> screen-space mapping.
     * Determined by sweeping a stylus across the full active area. */
    static const int16_t TS_MINX = 120;
    static const int16_t TS_MAXX = 900;
    static const int16_t TS_MINY = 110;
    static const int16_t TS_MAXY = 920;

    /* Temporal hysteresis debounce: 50 ms eliminates mechanical bounce
     * and prevents ISR-like spam from capacitive coupling on the overlay. */
    static const unsigned long TOUCH_DEBOUNCE_MS = 50;

    /* Rendering throttle: ~30 FPS cap prevents SPI/parallel bus saturation
     * and leaves CPU cycles for ternary simulation. */
    static const unsigned long SCREEN_UPDATE_MS  = 33;

    /* Simulation tick rate: 500 ms per ternary step. */
    static const unsigned long SIM_STEP_MS       = 500;
}

// -----------------------------------------------------------------------------
// Color Palette: Retro-Scientific Dark Mode (RGB565)
// -----------------------------------------------------------------------------
namespace Colors {
    static const uint16_t BG        = 0x0000; // Black
    static const uint16_t PRIMARY   = 0x07FF; // Neon Cyan  (0b00000 111111 11111)
    static const uint16_t SECONDARY = 0x041F; // Deep Blue-Cyan
    static const uint16_t ACCENT    = 0xF81F; // Magenta highlight
    static const uint16_t TEXT      = 0xFFFF; // White
    static const uint16_t DIM_TEXT  = 0x8410; // Gray
    static const uint16_t TRIT_N    = 0xF800; // Red   (-1)
    static const uint16_t TRIT_U    = 0xFFE0; // Yellow ( 0)
    static const uint16_t TRIT_P    = 0x07E0; // Green  (+1)
    static const uint16_t NAV_BG    = 0x1082; // Dark slate
    static const uint16_t NAV_ACT   = 0x07FF; // Cyan active
    static const uint16_t HEADER_BG = 0x0004; // Near-black with blue tint
}

// -----------------------------------------------------------------------------
// Inline Utilities
// -----------------------------------------------------------------------------
namespace Utils {
    /* Clamp an integer to inclusive bounds. Branch-predictor friendly on AVR. */
    static inline int16_t clamp(int16_t v, int16_t lo, int16_t hi) {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }

    /* Fast absolute value for int8_t. Avoids stdlib overhead. */
    static inline int8_t i8abs(int8_t v) {
        return v < 0 ? -v : v;
    }
}

// -----------------------------------------------------------------------------
// Ternary Logic Core: Balanced Trit ALU
// -----------------------------------------------------------------------------
class TernaryALU {
public:
    /* Balanced Trit: -1 (N), 0 (U), +1 (P) */
    enum Trit : int8_t { N = -1, U = 0, P = 1 };

    enum Operation : uint8_t {
        OP_MIN,     // Ternary AND / Minimum
        OP_MAX,     // Ternary OR  / Maximum
        OP_INV,     // Ternary NOT / Inversion
        OP_ADD,     // Balanced Ternary Addition with carry ripple
        OP_XOR,     // Ternary XOR (mod-3 inequality)
        OP_CYCLE,   // Ternary Successor / Cycle
        OP_COUNT
    };

    TernaryALU() { clearRegisters(); }

    /* Unary inversion: -(trit) */
    static inline Trit inv(Trit a) {
        return static_cast<Trit>(-static_cast<int8_t>(a));
    }

    /* Ternary AND: minimum of two trits */
    static inline Trit minOp(Trit a, Trit b) {
        return (a < b) ? a : b;
    }

    /* Ternary OR: maximum of two trits */
    static inline Trit maxOp(Trit a, Trit b) {
        return (a > b) ? a : b;
    }

    /* Balanced ternary full adder.
     * Carry is passed by reference and propagated to the next more-significant
     * trit, mimicking a ripple-carry adder. */
    static Trit add(Trit a, Trit b, Trit& carry) {
        int8_t sum = static_cast<int8_t>(a) + static_cast<int8_t>(b) + static_cast<int8_t>(carry);
        if (sum > 1) {
            carry = P;
            sum -= 3;
        } else if (sum < -1) {
            carry = N;
            sum += 3;
        } else {
            carry = U;
        }
        return static_cast<Trit>(sum);
    }

    /* Ternary XOR: P when inputs differ by 1; N when they differ by 2;
     * U when identical. This preserves balanced symmetry. */
    static Trit xorOp(Trit a, Trit b) {
        int8_t d = Utils::i8abs(static_cast<int8_t>(a) - static_cast<int8_t>(b));
        if (d == 0) return U;
        return (d == 2) ? N : P;
    }

    /* Successor function: cycles through N -> U -> P -> N */
    static Trit cycle(Trit a) {
        int8_t v = static_cast<int8_t>(a) + 1;
        if (v > 1) v = -1;
        return static_cast<Trit>(v);
    }

    /* Execute the selected operation across the full 3-trit wide registers.
     * For OP_INV and OP_CYCLE, regB is ignored (unary). */
    void execute(Operation op) {
        Trit carry = U;
        for (uint8_t i = 0; i < 3; i++) {
            Trit a = regA[i];
            Trit b = regB[i];
            Trit r;
            switch (op) {
                case OP_MIN:   r = minOp(a, b); break;
                case OP_MAX:   r = maxOp(a, b); break;
                case OP_INV:   r = inv(a);      break;
                case OP_ADD:   r = add(a, b, carry); break;
                case OP_XOR:   r = xorOp(a, b); break;
                case OP_CYCLE: r = cycle(a);    break;
                default:       r = U;           break;
            }
            regOut[i] = r;
        }
    }

    void clearRegisters() {
        for (uint8_t i = 0; i < 3; i++) {
            regA[i] = regB[i] = regOut[i] = U;
        }
    }

    /* Cycle a single trit in-place inside a register, then re-evaluate. */
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
        if (regIdx == 0)      regA[tritIdx] = val;
        else if (regIdx == 1) regB[tritIdx] = val;
        else                  regOut[tritIdx] = val;
    }

    /* Public register storage. Fixed 3-trit width keeps memory predictable. */
    Trit regA[3];
    Trit regB[3];
    Trit regOut[3];
};

// -----------------------------------------------------------------------------
// Touch Handler: Debouncing, Calibration & Bus Restoration
// -----------------------------------------------------------------------------
class TouchHandler {
public:
    TouchHandler()
        : ts(HW::TS_XP, HW::TS_YP, HW::TS_XM, HW::TS_YM, HW::TS_OHMS) {}

    void init() {
        _pressed = false;
        _tapped  = false;
        _lastPressed = false;
        _lastDebounce = 0;
        _x = _y = 0;
    }

    /* Non-blocking update. Must be called every loop() iteration.
     * Implements temporal hysteresis debouncing and maps raw ADC values
     * to screen-space coordinates. */
    void update() {
        _tapped = false;

        unsigned long now = millis();
        if (now - _lastDebounce < HW::TOUCH_DEBOUNCE_MS) {
            return; // Still inside debounce window
        }

        TSPoint p = ts.getPoint();

        /* CRITICAL: The MCUFRIEND shield shares A2 (LCD_RS/XM) and A1
         * (LCD_WR/YP) between the TFT parallel bus and the resistive touch
         * overlay. After every touch sample we must restore these pins to
         * OUTPUT mode so the next TFT draw command does not hang. */
        pinMode(HW::TS_XM, OUTPUT);
        pinMode(HW::TS_YP, OUTPUT);
        pinMode(HW::TS_XP, OUTPUT);
        digitalWrite(HW::TS_XP, LOW);
        digitalWrite(HW::TS_YP, HIGH);
        digitalWrite(HW::TS_XM, LOW);

        if (p.z < 10) {
            // Pressure below threshold -> release event
            _lastPressed = _pressed;
            _pressed = false;
            return;
        }

        /* Map raw ADC to screen coordinates. On MCUFRIEND shields the X and Y
         * axes are often swapped by the overlay routing; we map p.y to X and
         * p.x to Y to compensate. */
        int16_t tx = map(p.y, HW::TS_MINY, HW::TS_MAXY, 0, 240);
        int16_t ty = map(p.x, HW::TS_MINX, HW::TS_MAXX, 0, 320);

        tx = Utils::clamp(tx, 0, 239);
        ty = Utils::clamp(ty, 0, 319);

        _x = static_cast<uint16_t>(tx);
        _y = static_cast<uint16_t>(ty);

        bool wasPressed = _lastPressed;
        _lastPressed = _pressed;
        _pressed = true;

        /* Detect a fresh press (transition from open to closed). */
        if (!wasPressed && _pressed) {
            _tapped = true;
            _lastDebounce = now;
        }
    }

    bool isPressed() const { return _pressed; }
    bool isTapped()  const { return _tapped; }
    uint16_t getX()  const { return _x; }
    uint16_t getY()  const { return _y; }

    /* Navigation bar hit-test. The bar occupies y=[270,319].
     * Four buttons, 60 px each. Returns 0..3 or -1. */
    int8_t getNavRegion() const {
        if (_y < 270) return -1;
        uint8_t btn = _x / 60;
        if (btn > 3) btn = 3;
        return static_cast<int8_t>(btn);
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
    /* MCUFRIEND_kbv requires no pin arguments; the library derives the
     * correct data and control lines from the Mega 2560 shield layout. */
    UIEngine() : tft() {}

    void init() {
        /* Auto-detect the LCD controller ID (e.g., 0x9341 for ILI9341).
         * If detection fails, the library falls back to a safe default. */
        uint16_t id = tft.readID();
        tft.begin(id);
        tft.setRotation(0); // Portrait: 240 wide x 320 tall
        tft.fillScreen(Colors::BG);
        tft.setTextWrap(false);

        /* Diagnostic splash: report controller ID to Serial for field debugging. */
        Serial.print(F("TFT ID: 0x"));
        Serial.println(id, HEX);
    }

    void clearScreen() {
        tft.fillScreen(Colors::BG);
    }

    /* Clear only the content region, preserving header and nav bar. */
    void clearContent() {
        tft.fillRect(0, 30, 240, 240, Colors::BG);
    }

    /* Persistent header bar: 30 px tall. */
    void drawHeader(const char* subtitle) {
        tft.fillRect(0, 0, 240, 30, Colors::HEADER_BG);
        tft.drawFastHLine(0, 30, 240, Colors::PRIMARY);

        tft.setCursor(4, 4);
        tft.setTextColor(Colors::PRIMARY);
        tft.setTextSize(2);
        tft.print(F("TERNARY"));
        tft.setTextColor(Colors::TEXT);
        tft.print(F("RATOR"));

        tft.setCursor(4, 20);
        tft.setTextColor(Colors::DIM_TEXT);
        tft.setTextSize(1);
        tft.print(subtitle);

        /* Run-status indicator: green square when alive. */
        tft.fillRect(220, 8, 12, 12, Colors::TRIT_P);
        tft.drawRect(220, 8, 12, 12, Colors::TEXT);
    }

    /* Navigation bar: 50 px tall at bottom. Four equal sectors. */
    void drawNavBar(uint8_t activeIdx) {
        const char* labels[4] = {"ALU", "GATE", "SIM", "MENU"};
        tft.fillRect(0, 270, 240, 50, Colors::NAV_BG);
        tft.drawFastHLine(0, 270, 240, Colors::PRIMARY);

        for (uint8_t i = 0; i < 4; i++) {
            uint16_t x = i * 60;
            bool active = (i == activeIdx);
            uint16_t bg = active ? Colors::NAV_ACT : Colors::NAV_BG;
            uint16_t fg = active ? Colors::BG      : Colors::TEXT;

            tft.fillRect(x + 1, 271, 58, 48, bg);
            tft.drawRect(x + 2, 272, 56, 46, active ? Colors::TEXT : Colors::DIM_TEXT);

            tft.setCursor(x + 14, 288);
            tft.setTextColor(fg);
            tft.setTextSize(2);
            tft.print(labels[i]);
        }
    }

    /* Render a single trit as a neon-bordered cell. */
    void drawTritCell(uint16_t x, uint16_t y, uint16_t size,
                      TernaryALU::Trit t, bool highlight) {
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
            tft.drawRect(x + 1, y + 1, size - 2, size - 2, color);
        }

        tft.setCursor(x + size / 2 - 6, y + size / 2 - 8);
        tft.setTextColor(color);
        tft.setTextSize(2);
        tft.print(label);
    }

    /* ALU Mode: interactive 3-trit register visualizer. */
    void drawALU(const TernaryALU& alu, TernaryALU::Operation op) {
        clearContent();

        tft.setTextColor(Colors::PRIMARY);
        tft.setTextSize(1);
        tft.setCursor(10, 40);  tft.print(F("REG A"));
        tft.setCursor(10, 110); tft.print(F("REG B"));
        tft.setCursor(10, 180); tft.print(F("OP"));
        tft.setCursor(10, 250); tft.print(F("OUT"));

        // Register A (y = 55)
        for (uint8_t i = 0; i < 3; i++) {
            drawTritCell(60 + i * 55, 55, 45, alu.regA[i], false);
        }

        // Register B (y = 125)
        for (uint8_t i = 0; i < 3; i++) {
            drawTritCell(60 + i * 55, 125, 45, alu.regB[i], false);
        }

        // Operation indicator (y = 195)
        const char* opNames[] = {"MIN", "MAX", "INV", "ADD", "XOR", "CYC"};
        tft.setCursor(60, 195);
        tft.setTextColor(Colors::ACCENT);
        tft.setTextSize(2);
        tft.print(opNames[op]);
        tft.setTextColor(Colors::DIM_TEXT);
        tft.setCursor(130, 195);
        tft.setTextSize(1);
        tft.print(F("< tap A/B to cycle"));

        // Output Register (y = 235)
        for (uint8_t i = 0; i < 3; i++) {
            drawTritCell(60 + i * 55, 235, 45, alu.regOut[i], true);
        }

        // Decorative grid separators
        tft.drawFastHLine(10, 100, 220, Colors::DIM_TEXT);
        tft.drawFastHLine(10, 170, 220, Colors::DIM_TEXT);
        tft.drawFastHLine(10, 225, 220, Colors::DIM_TEXT);
    }

    /* Gate Configuration: truth table explorer. */
    void drawGateConfig(TernaryALU::Operation op) {
        clearContent();
        tft.setTextColor(Colors::PRIMARY);
        tft.setTextSize(1);
        tft.setCursor(10, 40);
        tft.print(F("TRUTH TABLE: "));

        const char* opNames[] = {"MIN", "MAX", "INV", "ADD", "XOR", "CYC"};
        tft.setTextColor(Colors::ACCENT);
        tft.setTextSize(2);
        tft.print(opNames[op]);

        bool isUnary = (op == TernaryALU::OP_INV || op == TernaryALU::OP_CYCLE);
        TernaryALU::Trit inputs[3] = {TernaryALU::N, TernaryALU::U, TernaryALU::P};

        if (isUnary) {
            tft.setTextColor(Colors::DIM_TEXT);
            tft.setCursor(40, 70);  tft.print(F("IN"));
            tft.setCursor(140, 70); tft.print(F("OUT"));
            tft.drawFastHLine(20, 82, 200, Colors::DIM_TEXT);

            for (uint8_t i = 0; i < 3; i++) {
                uint8_t y = 90 + i * 50;
                TernaryALU::Trit in = inputs[i];
                TernaryALU::Trit out = (op == TernaryALU::OP_INV)
                                       ? TernaryALU::inv(in)
                                       : TernaryALU::cycle(in);

                drawTritCell(30, y, 35, in, false);
                tft.setCursor(90, y + 12);
                tft.setTextColor(Colors::TEXT);
                tft.print(F("->"));
                drawTritCell(130, y, 35, out, true);
            }
        } else {
            // Binary operation grid (3 x 3)
            tft.setTextColor(Colors::DIM_TEXT);
            tft.setCursor(20, 70);  tft.print(F("A\\B"));
            tft.setCursor(70, 70);  tft.print(F("N"));
            tft.setCursor(120, 70); tft.print(F("0"));
            tft.setCursor(170, 70); tft.print(F("P"));
            tft.drawFastHLine(15, 82, 210, Colors::DIM_TEXT);

            for (uint8_t a = 0; a < 3; a++) {
                uint8_t y = 90 + a * 55;
                tft.setTextColor(Colors::DIM_TEXT);
                tft.setCursor(25, y + 12);
                tft.print((a == 0) ? "N" : (a == 1) ? "0" : "P");

                for (uint8_t b = 0; b < 3; b++) {
                    TernaryALU::Trit out;
                    switch (op) {
                        case TernaryALU::OP_MIN:
                            out = TernaryALU::minOp(inputs[a], inputs[b]);
                            break;
                        case TernaryALU::OP_MAX:
                            out = TernaryALU::maxOp(inputs[a], inputs[b]);
                            break;
                        case TernaryALU::OP_ADD: {
                            TernaryALU::Trit c = TernaryALU::U;
                            out = TernaryALU::add(inputs[a], inputs[b], c);
                            break;
                        }
                        case TernaryALU::OP_XOR:
                            out = TernaryALU::xorOp(inputs[a], inputs[b]);
                            break;
                        default:
                            out = TernaryALU::U;
                            break;
                    }
                    drawTritCell(60 + b * 50, y, 40, out, false);
                }
            }
        }
    }

    /* Simulation Mode: animated ternary wave / ring counter. */
    void drawSimulation(uint8_t step, uint8_t phase) {
        // Partial redraw: only the wave region to minimize bus traffic.
        tft.fillRect(0, 50, 240, 215, Colors::BG);

        tft.setTextColor(Colors::PRIMARY);
        tft.setTextSize(1);
        tft.setCursor(10, 40);
        tft.print(F("TERNARY COUNTER / WAVE"));

        const uint8_t baseY = 120;
        const uint8_t amp   = 40;

        // Draw 20 visible samples of a ternary sequence.
        for (uint8_t i = 0; i < 20; i++) {
            uint8_t idx = (step + i) % 27; // 3^3 states
            int8_t val = (idx % 3) - 1;    // map to {-1, 0, +1}

            uint16_t x = 10 + i * 11;
            int16_t  y = baseY - (val * amp);

            uint16_t color = (val == -1) ? Colors::TRIT_N
                           : (val == 0)  ? Colors::TRIT_U
                                         : Colors::TRIT_P;

            tft.fillRect(x, y - 3, 8, 6, color);

            if (i < 19) {
                int8_t nextVal = ((idx + 1) % 3) - 1;
                int16_t y2 = baseY - (nextVal * amp);
                tft.drawLine(x + 4, y, x + 11, y2, Colors::DIM_TEXT);
            }
        }

        // Phase / step telemetry
        tft.setCursor(10, 180);
        tft.setTextColor(Colors::TEXT);
        tft.setTextSize(1);
        tft.print(F("Phase: "));
        tft.print(phase);
        tft.print(F("  Step: "));
        tft.print(step);

        // Scrollling trit register derived from step
        uint8_t s = step % 27;
        uint8_t simReg[3];
        simReg[0] = (s / 9) % 3;
        simReg[1] = (s / 3) % 3;
        simReg[2] = s % 3;

        for (uint8_t i = 0; i < 3; i++) {
            TernaryALU::Trit t = static_cast<TernaryALU::Trit>(simReg[i] - 1);
            drawTritCell(60 + i * 55, 200, 45, t, true);
        }
    }

    /* Menu / splash screen. */
    void drawMenu() {
        clearContent();
        tft.setTextColor(Colors::PRIMARY);
        tft.setTextSize(2);
        tft.setCursor(30, 60);
        tft.print(F("TERNARYRATOR"));

        tft.setTextColor(Colors::DIM_TEXT);
        tft.setTextSize(1);
        tft.setCursor(20, 90);
        tft.print(F("Balanced Ternary Logic Simulator"));
        tft.setCursor(20, 105);
        tft.print(F("v1.0 | Mega 2560 | MCUFRIEND"));

        tft.setCursor(20, 140);
        tft.setTextColor(Colors::TEXT);
        tft.print(F("Select mode from nav bar below:"));
        tft.setCursor(30, 160);
        tft.setTextColor(Colors::TRIT_P);
        tft.print(F("ALU: Interactive Arithmetic"));
        tft.setCursor(30, 175);
        tft.setTextColor(Colors::TRIT_U);
        tft.print(F("GATE: Truth Table Explorer"));
        tft.setCursor(30, 190);
        tft.setTextColor(Colors::TRIT_N);
        tft.print(F("SIM: Ternary Wave Animation"));
    }

    /* Visual feedback flash. Intentionally short blocking (30 ms) for UX;
     * all other logic remains non-blocking. */
    void flashRegion(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
        tft.drawRect(x, y, w, h, Colors::ACCENT);
        delay(30);
        tft.drawRect(x, y, w, h, Colors::BG); // erase flash
    }

    /* Direct low-level access for rare diagnostic use. */
    MCUFRIEND_kbv* getTft() { return &tft; }

private:
    MCUFRIEND_kbv tft;
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

    /* Dirty flag is consumed on read (one-shot). */
    bool isDirty() {
        bool d = _dirty;
        _dirty = false;
        return d;
    }

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
uint8_t simStep = 0;
uint8_t simPhase = 0;
unsigned long lastSimUpdate = 0;
unsigned long lastScreenUpdate = 0;

// -----------------------------------------------------------------------------
// Setup & Initialization
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    // Optional serial handshake for field diagnostics; non-blocking after 2 s.
    unsigned long serialTimeout = millis();
    while (!Serial && (millis() - serialTimeout < 2000)) { /* spin */ }

    // Initialize display and touch subsystems
    ui.init();
    touch.init();

    // Boot splash
    ui.clearScreen();
    ui.drawHeader(F("SYSTEM BOOT"));
    ui.drawMenu();
    ui.drawNavBar(3); // MENU active

    Serial.println(F("========================================"));
    Serial.println(F(" Ternaryrator Initialized"));
    Serial.println(F("----------------------------------------"));
    Serial.println(F(" Mem model: Static only, no heap"));
    Serial.println(F(" TFT drv:   MCUFRIEND_kbv (parallel)"));
    Serial.println(F(" Touch:     4-wire resistive"));
    Serial.println(F("========================================"));
}

// -----------------------------------------------------------------------------
// Main Loop: Non-Blocking FSM
// -----------------------------------------------------------------------------
void loop() {
    unsigned long now = millis();

    // 1. Input Sampling (always active)
    touch.update();

    // 2. Global Navigation Handling (highest priority)
    if (touch.isTapped()) {
        int8_t navBtn = touch.getNavRegion();
        if (navBtn >= 0) {
            StateManager::State targetState;
            switch (navBtn) {
                case 0: targetState = StateManager::STATE_ALU;  break;
                case 1: targetState = StateManager::STATE_GATE; break;
                case 2: targetState = StateManager::STATE_SIM;  break;
                case 3: targetState = StateManager::STATE_MENU; break;
                default: targetState = StateManager::STATE_MENU; break;
            }
            if (targetState != state.getState()) {
                state.transitionTo(targetState);
            }
        }
    }

    // 3. State-Specific Logic & Interaction
    handleStateLogic(now);

    // 4. Rendering (throttled to ~30 FPS to prevent parallel bus saturation)
    if (now - lastScreenUpdate >= HW::SCREEN_UPDATE_MS) {
        lastScreenUpdate = now;
        renderState();
    }
}

// -----------------------------------------------------------------------------
// State Logic Handlers
// -----------------------------------------------------------------------------
void handleStateLogic(unsigned long now) {
    switch (state.getState()) {
        case StateManager::STATE_ALU:  handleALULogic();  break;
        case StateManager::STATE_GATE: handleGateLogic(); break;
        case StateManager::STATE_SIM:  handleSimLogic(now); break;
        default: break; // MENU is static
    }
}

/* ALU Mode: tap a trit cell to cycle it; tap OP label to rotate operation. */
void handleALULogic() {
    if (!touch.isTapped()) return;

    uint16_t tx = touch.getX();
    uint16_t ty = touch.getY();

    // Ignore nav bar taps (already handled globally)
    if (ty >= 270) return;

    // Register A hit-test (y = 55..100, x = 60 + i*55)
    if (ty >= 55 && ty <= 100) {
        for (uint8_t i = 0; i < 3; i++) {
            uint16_t cx = 60 + i * 55;
            if (tx >= cx && tx <= cx + 45) {
                alu.cycleRegister(0, i);
                alu.execute(currentOp);
                state.markDirty();
                return;
            }
        }
    }

    // Register B hit-test (y = 125..170)
    if (ty >= 125 && ty <= 170) {
        for (uint8_t i = 0; i < 3; i++) {
            uint16_t cx = 60 + i * 55;
            if (tx >= cx && tx <= cx + 45) {
                alu.cycleRegister(1, i);
                alu.execute(currentOp);
                state.markDirty();
                return;
            }
        }
    }

    // Operation selector hit-test (y = 185..220)
    if (ty >= 185 && ty <= 220) {
        currentOp = static_cast<TernaryALU::Operation>(
            (currentOp + 1) % TernaryALU::OP_COUNT);
        alu.execute(currentOp);
        state.markDirty();
    }
}

/* Gate Mode: any tap in the content area cycles the displayed operation. */
void handleGateLogic() {
    if (!touch.isTapped()) return;
    if (touch.getY() >= 270) return; // Nav only

    currentOp = static_cast<TernaryALU::Operation>(
        (currentOp + 1) % TernaryALU::OP_COUNT);
    state.markDirty();
}

/* Simulation Mode: autonomous time-driven stepper. */
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

    // Header and nav are redrawn on every state transition
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
        default:                       return "MAIN MENU";
    }
}