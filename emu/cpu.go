package emu

import (
    "fmt"
    "github.com/tedkotz/trich-ternary/core"
)

// --- Alles hier vereint, damit Go nicht mehr "undefined" schreit ---

type Memory struct {
    cells []core.Trit
}

func NewMemory(size int) *Memory {
    return &Memory{cells: make([]core.Trit, size)}
}

type RegisterBank struct {
    Data [][]core.Trit
}

func NewRegisterBank(numRegs, wordSize int) *RegisterBank {
    data := make([][]core.Trit, numRegs)
    for i := range data {
        data[i] = make([]core.Trit, wordSize)
    }
    return &RegisterBank{Data: data}
}

type CPUState struct {
    Registers [][]core.Trit
    PC        int
    CarryFlag core.Trit
    Halted    bool
}

type CPU struct {
    State     *CPUState
    Memory    *Memory
    Registers *RegisterBank
}

func NewCPU(memSize, numRegs, wordSize int) *CPU {
    return &CPU{
        State:     &CPUState{Registers: make([][]core.Trit, numRegs)},
        Memory:    NewMemory(memSize),
        Registers: NewRegisterBank(numRegs, wordSize),
    }
}

func (c *CPU) Add(target, src1, src2 int) {
    sum, _ := core.FullAdder(c.Registers.Data[src1][0], c.Registers.Data[src2][0], core.Zero)
    c.Registers.Data[target][0] = sum
    fmt.Printf("Executed ADD: R%d = R%d + R%d\n", target, src1, src2)
}

func (c *CPU) Sub(target, src1, src2 int) {
    val2 := c.Registers.Data[src2][0]
    negVal2 := core.Trit(-val2.ToInt())
    sum, _ := core.FullAdder(c.Registers.Data[src1][0], negVal2, core.Zero)
    c.Registers.Data[target][0] = sum
    fmt.Printf("Executed SUB: R%d = R%d - R%d\n", target, src1, src2)
}