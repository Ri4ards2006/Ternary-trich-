# Core Logic: Ternary Hardware Synthesis

This directory contains the foundational logic layer of the `trich-ternary` architecture. It provides the mathematical primitives, gate definitions, and combinational logic necessary to emulate a balanced ternary computing system (Base-3: {-1, 0, 1}).

## Architecture Overview

The core module is designed to map directly to physical hardware gates. It follows a strict hierarchical approach:

* **`types.go`**: Defines the `Trit` primitive (int8). This is the atomic unit of the entire machine.
* **`gates.go`**: Contains the basic logic gates (NAND, NOT, OR). These functions simulate the behavior of individual transistor circuits.
* **`algebra.go`**: Contains complex arithmetic logic (Adders, Multipliers). This is where primitives are "stacked" to form meaningful computing structures.



## Implementation Details

### The Trit System
Unlike binary, our system utilizes a balanced ternary approach:
* **PosOne (+)**: $1$
* **Zero (0)**: $0$
* **NegOne (-)**: $-1$

### Logic Primitives
The core uses **Min-NAND** as the universal gate. All other operations are derived from this to simplify the eventual transition to physical hardware:

$$\text{NAND}(a, b) = -(\min(a, b))$$

## Development Guidelines

1.  **Strict Typing**: Never use raw integers for logic operations. Always use the `Trit` type to ensure the system stays within the balanced ternary domain.
2.  **No Unused Imports**: The Go compiler is strict. Clean up your imports before committing.
3.  **Test-Driven Development**: Every new gate or algebraic function must have a corresponding test case in `logic_test.go`.

## Testing

To verify the integrity of your logic gates, run the test suite from within this directory:

```bash
go test -v .
``` 

Expected output:

Plaintext
=== RUN   Test...
--- PASS: ... (0.00s)
PASS
ok      [github.com/tedkotz/trich-ternary/core](https://github.com/tedkotz/trich-ternary/core)    0.001s

## Hardware Mapping Note
The functions defined here serve as the blueprint for the PCB implementation. If the logic fails here, it will fail on the silicon. Respect the logic.