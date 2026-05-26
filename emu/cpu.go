package emu

import (
	"fmt"
	"github.com/tedkotz/trich-ternary/core"
)

// CPUState hält die Hardware-Informationen.
type CPUState struct {
	Registers  [8]core.Trit
	PC         int
	CarryFlag  core.Trit // Wichtig für professionelle Arithmetik
	Halted     bool
}

// CPU repräsentiert den Interpreter, der den Status manipuliert.
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

// Tick führt einen einzelnen CPU-Zyklus aus.
func (c *CPU) Tick() {
	if c.State.Halted { return }
	// Hier würde der "Fetch-Decode" Zyklus stehen.
	c.State.PC++
}