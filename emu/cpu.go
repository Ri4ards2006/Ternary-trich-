package emu

import (
    "fmt"
    "github.com/tedkotz/trich-ternary/core"
)

type CPU struct {
    State     *CPUState
    Registers *RegisterBank
}

func (c *CPU) Sub(target, src1, src2 int) {
    // Einfache Subtraktion: A - B
    // In Ternär: A - B = A + (-B)
    val2 := c.Registers.Data[src2][0]
    negVal2 := core.Trit(-val2.ToInt()) // Simple Negation
    
    sum, _ := core.FullAdder(c.Registers.Data[src1][0], negVal2, core.Zero)
    c.Registers.Data[target][0] = sum
    
    fmt.Printf("Executed SUB: R%d = R%d - R%d\n", target, src1, src2)
}

func (c *CPU) Add(target, src1, src2 int) {
    sum, _ := core.FullAdder(c.Registers.Data[src1][0], c.Registers.Data[src2][0], core.Zero)
    c.Registers.Data[target][0] = sum
    fmt.Printf("Executed ADD: R%d = R%d + R%d\n", target, src1, src2)
}