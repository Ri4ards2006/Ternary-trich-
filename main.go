package main

import (
	"fmt"
	"github.com/tedkotz/trich-ternary/core"
	"github.com/tedkotz/trich-ternary/emu"
)

func main() {
	// Initialize CPU with 256 cells of memory
	cpu := emu.NewCPU(256)
	
	fmt.Println("--- Ternary VM Booting... ---")

	// Load values: R1 = 1, R2 = 1
	cpu.Registers[1] = core.PosOne
	cpu.Registers[2] = core.PosOne
	
	fmt.Printf("Loaded: R1=%s, R2=%s\n", cpu.Registers[1], cpu.Registers[2])

	// Add R1 + R2 -> Result in R0
	cpu.Add(0, 1, 2)

	// Check result
	result := cpu.Registers[0]
	fmt.Printf("CPU Execution finished. R0 = %s\n", result)
	
	fmt.Println("--- System Halt ---")
}
