package main

import (
	"fmt"
	"github.com/tedkotz/trich-ternary/core"
	"github.com/tedkotz/trich-ternary/emu"
)// In emu/cpu.go
var OpCodes = map[string]func(c *CPU, target, src1, src2 int){
    "ADD": (*CPU).Add,
    "SUB": (*CPU).Sub,
    // Hier kannst du unendlich viele Befehle hinzufügen!
}


func main() {
	cpu := emu.NewCPU(256)
	
	fmt.Println("--- Ternary VM Booting... ---")

	// Zugriff über cpu.State.Registers
	cpu.State.Registers[1] = core.PosOne
	cpu.State.Registers[2] = core.PosOne
	
	fmt.Printf("Loaded: R1=%s, R2=%s\n", cpu.State.Registers[1], cpu.State.Registers[2])

	// ADD Methode bleibt gleich
	cpu.Add(0, 1, 2)

	result := cpu.State.Registers[0]
	fmt.Printf("CPU Execution finished. R0 = %s\n", result)
	
	fmt.Println("--- System Halt ---")
}