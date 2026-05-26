package main

import (
	"fmt"
	"github.com/tedkotz/trich-ternary/core"
	"github.com/tedkotz/trich-ternary/emu"
)

func main() {
	// 1. Initialisiere die Komponenten
	cpu := emu.NewCPU(256) // CPU mit 256 Speicherzellen RAM
	
	fmt.Println("--- Ternary VM Booting... ---")

	// 2. Lade Daten in die Register (z.B. R1 = 1, R2 = 1)
	cpu.Registers[1] = core.PosOne
	cpu.Registers[2] = core.PosOne
	
	fmt.Printf("Loaded: R1=%s, R2=%s\n", cpu.Registers[1], cpu.Registers[2])

	// 3. Führe die Addition aus (Addiere R1 + R2 -> speichere in R0)
	cpu.Add(0, 1, 2)

	// 4. Prüfe das Ergebnis
	result := cpu.Registers[0]
	fmt.Printf("CPU Execution finished. R0 = %s\n", result)
	
	fmt.Println("--- System Halt ---")
}
