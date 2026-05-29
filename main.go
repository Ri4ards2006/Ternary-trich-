package emu

import (
    "fmt"
    "github.com/tedkotz/trich-ternary/core"
)

// CPUState muss hier sein, damit cpu.go es kennt
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

// ACHTUNG: Die Methoden müssen GROSS geschrieben sein (Add, Sub), 
// sonst sieht isa.go sie nicht!

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