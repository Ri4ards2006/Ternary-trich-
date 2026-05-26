# trich-ternary
*Non-binary computing: From mathematical theory to silicon logic.*

---

## 📜 The Lore
In the 1950s, the **Setun** computer proved that binary is not the only way. Balanced Ternary (Base-3) uses $\{-1, 0, 1\}$ as its fundamental digits. This system eliminates two's complement, simplifies multiplication, and offers higher information density. `trich-ternary` is an ongoing journey to revive and evolve this architecture.

## 🛠 Tech Stack & Philosophy
We combine the speed of systems programming with the analytical power of data science.

| Layer | Tool | Purpose |
| :--- | :--- | :--- |
| **Logic Core** | **Go** | Fast emulation, unit testing, architecture definition. |
| **Toolchain** | **Rust** | High-performance assembler, memory-safe drivers. |
| **Analysis** | **Python** | Signal plotting, data visualization, math modeling. |
| **Hardware** | **FPGA/PCB** | Physical realization of ternary gates (Verilog/SMD). |

## 📐 Ternary Logic Reference
Standard binary logic is insufficient here. We implement custom gates based on the set $\{-1, 0, 1\}$.

| Input A | Input B | Ternary NAND |
| :---: | :---: | :---: |
| +1 | +1 | -1 |
| +1 | 0 | 1 |
| 0 | -1 | 1 |

*(Note: Logic tables are under active development in `/docs`.)*

## 🚀 Roadmap
- [ ] **Phase 1: Foundation (Go)**
    - Implement `Trit` type and basic arithmetic.
    - Define Instruction Set Architecture (ISA).
- [ ] **Phase 2: Compiler/Assembler (Rust)**
    - First experience with Rust: Building a cross-assembler.
    - Creating a memory-safe bytecode translator.
- [ ] **Phase 3: Silicon/FPGA**
    - Porting logic gates to Verilog/VHDL.
    - Prototyping on FPGA boards.
- [ ] **Phase 4: Physical Hardware**
    - Designing a 4-Trit Adder using 30-60 SMD transistors.
    - Signal integrity testing via Oscilloscope.

## 📊 Evaluation Metrics
How do we know if we are winning?
1. **Gate Efficiency:** Can we achieve the same output with fewer transistors than binary?
2. **Speed:** Emulation cycles per second.
3. **Accuracy:** Correlation between software simulation and physical FPGA/PCB gate behavior.

---
*“Complexity is the enemy of execution. Elegance is the key to architecture.”*
