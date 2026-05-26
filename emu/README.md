# Emulator: Ternary Virtual Machine

This directory contains the runtime environment for the `trich-ternary` architecture. 
It simulates the execution cycle, register state, and memory bus of a balanced 
ternary (Base-3) machine.

## Components
- **CPU**: The core executor. Manages the instruction cycle (Fetch-Decode-Execute).
- **Registers**: 8 specialized slots (R0-R7) for storing `Trit` values.
- **Memory**: A linear heap of `Trit` values, acting as the system's RAM.

## Usage
The emulator imports the `core` package to utilize the `FullAdder` and gate logic 
for all arithmetic operations. 
To start the machine:
1. Initialize the `CPU` struct.
2. Load values into the `Registers`.
3. Run the `Execute()` loop.
