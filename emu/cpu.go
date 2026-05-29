package emu

import (
	"fmt"
	"github.com/tedkotz/trich-ternary/core"
)

type CPUState struct {
	Registers [8]core.Trit
	PC        int
	CarryFlag core.Trit
	Halted    bool
}

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
	fmt.Printf("Executed ADD: R%d = R%d + R%d (Result: %s, Carry: %s)\n", 
		target, src1, src2, sum, carry)
}