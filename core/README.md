# Core Logic: Ternary Hardware Synthesis

The foundation of the `trich-ternary` architecture. This package provides the mathematical primitives, gate definitions, and combinational logic necessary to simulate a balanced ternary ($\{-1, 0, 1\}$) system.

## Mathematical Primitives
- **Trit**: The atomic unit. Represented as `int8` with values $\{-1, 0, 1\}$.
- **NAND Logic**: Uses Min-NAND as the universal gate. All operations ($\text{NOT, AND, OR, XOR}$) are synthesized from this.
- **Full Adder**: A complete algebraic block that computes $Sum$ and $Carry$ based on balanced ternary arithmetic.

## File Structure
- `types.go`: Primitive definitions and string serialization.
- `gates.go`: Fundamental logic gates (transistor-level simulation).
- `algebra.go`: Arithmetic operations (Adders/Multipliers).
- `logic_test.go`: 100% test coverage suite for logical integrity.

## Testing
Run the following in the terminal:
```bash
go test -v .