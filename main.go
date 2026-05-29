package main

import (
    "fmt"
    "github.com/tedkotz/trich-ternary/core"
    "github.com/tedkotz/trich-ternary/emu"
)

func main() {
    fmt.Println("--- Ternary Hardware Configurator ---")
    
    // CPU initialisieren: 256 Speicher, 8 Register, 4 Trits Wortbreite
    cpu := emu.NewCPU(256, 8, 4) 
    
    fmt.Print("Wie viele Additions-Gatter willst du kaskadieren? ")
    var gateCount int
    fmt.Scan(&gateCount)

    // Initialisiere Daten
    cpu.Registers.Data[1][0] = core.PosOne
    cpu.Registers.Data[2][0] = core.PosOne
    
    // Simulation starten
    for i := 0; i < gateCount; i++ {
        cpu.Add(0, 1, 2)
        fmt.Printf("Gatter %d: R0 = %s\n", i+1, cpu.Registers.Data[0][0])
    }
}