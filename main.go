package main

import (
    "fmt"
    "github.com/tedkotz/trich-ternary/core"
    "github.com/tedkotz/trich-ternary/emu"
)

func main() {
    // Da Memory & RegisterBank jetzt im Paket emu sind, 
    // und NewCPU sie korrekt initialisiert, brauchst du sie hier nicht direkt
    cpu := emu.NewCPU(256, 8, 4)

    cpu.Registers.Data[1][0] = core.PosOne
    cpu.Registers.Data[2][0] = core.PosOne

    cpu.Add(0, 1, 2)
    fmt.Println("Berechnung abgeschlossen.")
}