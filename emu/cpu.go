package emu

import (
    "fmt"
    "github.com/tedkotz/trich-ternary/core"
)

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

func (c *CPU) Sub(target, src1, src2 int) {
    // A - B = A + (-B). Da wir hier nur ein Trit haben: -val
    val2 := c.Registers.Data[src2][0]
    negVal2 := core.Trit(-val2.ToInt()) 
    
    sum, _ := core.FullAdder(c.Registers.Data[src1][0], negVal2, core.Zero)
    c.Registers.Data[target][0] = sum
    fmt.Printf("Executed SUB: R%d = R%d - R%d\n", target, src1, src2)
}