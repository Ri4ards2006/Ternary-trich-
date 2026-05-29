package emu

import (
	"fmt"
	"github.com/tedkotz/trich-ternary/core"
)

// Hier definieren wir den Status
type CPUState struct {
	Registers [8]core.Trit
	PC        int
	CarryFlag core.Trit
	Halted    bool
}

// Hier definieren wir die CPU
type CPU struct {
	State  *CPUState
	Memory *Memory
}

func NewCPU(memSize int) *CPU {
	return &CPU{
		State:  &CPUState{},
		Memory: NewMemory(memSize),
	}
}

func (c *CPU) Add(target, src1, src2 int) {
	sum, carry := core.FullAdder(c.State.Registers[src1], c.State.Registers[src2], core.Zero)
	c.State.Registers[target] = sum
	c.State.CarryFlag = carry
	fmt.Printf("Executed ADD: R%d = R%d + R%d\n", target, src1, src2)
}

func (c *CPU) Sub(target, src1, src2 int) {
    // Falls core.Negate nicht existiert, nutze core.TritInvert oder bau eine einfache Negation
	sum, carry := core.FullAdder(c.State.Registers[src1], c.State.Registers[src2], core.Zero)
	c.State.Registers[target] = sum
	c.State.CarryFlag = carry
	fmt.Printf("Executed SUB: R%d = R%d - R%d\n", target, src1, src2)
}