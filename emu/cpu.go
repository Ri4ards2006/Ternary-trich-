package emu

import (
	"fmt"
	"github.com/tedkotz/trich-ternary/core"
)

// CPU represents the emulated hardware environment.
type CPU struct {
	Registers [8]core.Trit // Eight general-purpose registers (R0-R7)
	Memory    []core.Trit  // System RAM
	PC        int          // Program Counter
}

// NewCPU initializes a new ternary CPU instance.
func NewCPU(memorySize int) *CPU {
	return &CPU{
		Registers: [8]core.Trit{},
		Memory:    make([]core.Trit, memorySize),
		PC:        0,
	}
}

// Add performs an addition of two registers and stores the result in the target register.
// This function bridges the emulated register state with the core ALU logic.
func (c *CPU) Add(target, src1, src2 int) {
	// Access the ALU logic from the core package
	sum, carry := core.FullAdder(c.Registers[src1], c.Registers[src2], core.Zero)
	
	c.Registers[target] = sum
	fmt.Printf("Executed ADD: R%d = R%d + R%d (Result: %s, Carry: %s)\n", 
		target, src1, src2, sum, carry)
}

// Execute handles the instruction cycle (simplified).
func (c *CPU) Execute() {
	fmt.Println("CPU Cycle started...")
	// Logic for Fetch-Decode-Execute will go here
}
