# 🚀 Ternary Virtual Machine (TVM)

The **TVM** is a highly modular emulation environment for balanced ternary calculations (Base-3: `{-1, 0, +1}`). Unlike traditional binary systems, this architecture leverages the mathematical elegance of ternary logic, providing symmetric representation of positive and negative numbers.

## 🛠 Architectural Overview

| Component | Responsibility |
| --- | --- |
| **CPU** | Core execution unit; manages the Fetch-Decode-Execute cycle. |
| **CPUState** | Maintains current register values, Program Counter (PC), and status flags. |
| **Memory** | A linear heap of `Trit` values, acting as system RAM. |
| **ALU** | Arithmetic Logic Unit; performs state transformations (Addition, Subtraction). |
| **ISA** | Instruction Set Architecture; defines the supported operation codes (OpCodes). |

---

## 📐 Mathematical Basis: Balanced Ternary

Unlike binary (`0, 1`), balanced ternary uses `{-1, 0, +1}`.

> **Note:** Our implementation utilizes `FullAdder` logic to balance states. A `Carry` in this system can be negative or positive, allowing for highly efficient arithmetic without complex two's complement conversions.

---

## ⚙️ Dynamic Pipeline Configuration

The system allows for iterative gate cascading. You can define the depth of your pipeline during runtime, turning the machine from a simple adder into a multi-stage logic circuit.

### Operation Lifecycle

1. **Config:** User defines the number of gates (pipeline depth) via the console.
2. **Dispatch:** The system utilizes a `map`-based dispatcher to resolve instructions.
3. **Execution:** The iterative `performCalculation` loop processes the `Trit` stream.

---

## 🚀 Usage Guide

To run your own simulation, follow these steps:

### 1. Requirements

Ensure you have the latest Go compiler installed and the project dependencies resolved:

```bash
go mod tidy

```

### 2. Execution

Run the orchestrator directly from the root directory:

```bash
go run main.go

```

### 3. Pipeline Example

When prompted by the **Hardware Configurator**, input the desired number of gate cycles:

* **Input:** `4`
* **Output:**
* `Gatter 1: Intermediate = -, Carry = +`
* `Gatter 2: Intermediate = 0, Carry = 0`
* ... and so on.



---

## 🔮 Roadmap: The Next Evolution

To make the TVM truly programmable, the next development phase will focus on:

* **Gate Library Expansion**: Implementing a `GateLibrary` map to allow switching between `ADD`, `SUB`, `NAND`, and `CONSENSUS` gates at runtime.
* **Instruction Pipelining**: Moving from hardcoded loops to a program loader that reads a sequence of instructions from `Memory`.

---

