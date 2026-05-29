package main

import (
	"fmt"
	"github.com/tedkotz/trich-ternary/core"
	"github.com/tedkotz/trich-ternary/emu"
)

func main() {
	// Initialize CPU
	cpu := emu.NewCPU(256)
	
	fmt.Println("--- Ternary VM Booting... ---")

	// Daten in die Register laden
	cpu.State.Registers[1] = core.PosOne
	cpu.State.Registers[2] = core.PosOne
	
	fmt.Printf("Loaded: R1=%s, R2=%s\n", cpu.State.Registers[1], cpu.State.Registers[2])

	// ADD Befehl ausführen
	cpu.Add(0, 1, 2)

	// Ergebnis prüfen
	result := cpu.State.Registers[0]
	fmt.Printf("CPU Execution finished. R0 = %s\n", result)
	
	fmt.Println("--- System Halt ---")
}