package emu

import "fmt"

// RenderDashboard zeichnet den aktuellen CPU-Zustand übersichtlich.
func (c *CPU) RenderDashboard() {
	fmt.Println("╔══════════════════════════════════════════════╗")
	fmt.Printf("║ [STATUS] PC: %04d | Carry: %s | Halted: %t ║\n", 
		c.State.PC, c.State.CarryFlag, c.State.Halted)
	fmt.Println("╠══════════════════════════════════════════════╣")
	fmt.Printf("║ REGISTERS: R0:%s R1:%s R2:%s R3:%s R4:%s... ║\n", 
		c.State.Registers[0], c.State.Registers[1], c.State.Registers[2], 
		c.State.Registers[3], c.State.Registers[4])
	fmt.Println("╚══════════════════════════════════════════════╝")
}