package main

import (
    "fmt"
    "github.com/tedkotz/trich-ternary/core"
    "github.com/tedkotz/trich-ternary/emu"
)

func main() {
    cpu := emu.NewCPU(256, 8, 4)
    
    // Beispiel-Werte setzen
    cpu.Registers.Data[1][0] = core.PosOne
    cpu.Registers.Data[2][0] = core.NegOne

    fmt.Println("--- Ternary Hardware Configurator ---")
    fmt.Println("Wähle Operation: ADD oder SUB")
    
    var op string
    fmt.Scanln(&op)

    switch op {
    case "ADD":
        cpu.Add(0, 1, 2)
    case "SUB":
        cpu.Sub(0, 1, 2)
    default:
        fmt.Println("Unbekannte Operation!")
    }
    
    fmt.Printf("Ergebnis in R0: %s\n", cpu.Registers.Data[0][0])
}