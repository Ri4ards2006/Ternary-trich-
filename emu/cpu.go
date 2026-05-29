package emu

import (
    "fmt"
    "github.com/tedkotz/trich-ternary/core"
)

type CPUState struct {
    Registers [][]core.Trit // Jetzt ein Slice von Slices
    PC        int
    CarryFlag core.Trit
    Halted    bool
}

type CPU struct {
    State     *CPUState
    Memory    *Memory
    ALU       *ALU
    Registers *RegisterBank
}

func NewCPU(memSize, numRegs, wordSize int) *CPU {
    return &CPU{
        State:     &CPUState{Registers: make([][]core.Trit, numRegs)},
        Memory:    NewMemory(memSize),
        ALU:       &ALU{},
        Registers: NewRegisterBank(numRegs, wordSize),
    }
}

// Add für Word-Slices (Die Kaskade)
func (c *CPU) Add(target, src1, src2 int) {
    w1 := c.Registers.Data[src1]
    w2 := c.Registers.Data[src2]
    
    // Hier wird das Wort addiert (Platzhalter für die Kaskaden-Logik)
    fmt.Printf("Executed ADD: R%d = R%d + R%d (Word-Breite: %d)\n", 
        target, src1, src2, len(w1))
}