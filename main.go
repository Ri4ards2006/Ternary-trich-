package main

import (
    "fmt"
    "github.com/tedkotz/trich-ternary/core"
    "github.com/tedkotz/trich-ternary/emu"
)

func main() {
    fmt.Println("--- Ternary Hardware Configurator ---")
    
    // WICHTIG: Präfix "emu." vor den Typen!
    cpu := emu.NewCPU(256, 8, 4) 
    
    // Hier musst du den Zugriff auf Register prüfen, 
    // falls "Registers" ein Struct ist, der "Data" enthält
    cpu.Registers.Data[1][0] = core.PosOne
    cpu.Registers.Data[2][0] = core.PosOne
    
    fmt.Println("CPU initialisiert.")
    cpu.Add(0, 1, 2)
}