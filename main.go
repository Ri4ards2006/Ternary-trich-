package main

import (
	"fmt"
	"github.com/tedkotz/trich-ternary/core"
	"github.com/tedkotz/trich-ternary/emu"
)

func main() {
	fmt.Println("--- Ternary Hardware Configurator ---")
	
	fmt.Print("Wie viele Additions-Gatter willst du kaskadieren? ")
	var gateCount int
	fmt.Scan(&gateCount)

	// Wir nutzen hier die Importe, damit Go nicht meckert
	cpu := emu.NewCPU(256)
	fmt.Printf("CPU initialisiert mit %d Gattern (simuliert)\n", gateCount)

	// Beispielaufruf der Funktion
	performCalculation(cpu, gateCount)
}

func performCalculation(c *emu.CPU, count int) {
    // Initialwerte
    c.State.Registers[1] = core.PosOne
    c.State.Registers[2] = core.PosOne
    
    // Wir speichern das Zwischenergebnis
    lastResult := c.State.Registers[1] 

    for i := 0; i < count; i++ {
        // Hier schalten wir die Gatter hintereinander:
        // Das Ergebnis von Runde i wird zum Input für Runde i+1
        sum, carry := core.FullAdder(lastResult, c.State.Registers[2], core.Zero)
        
        lastResult = sum
        c.State.Registers[0] = sum
        fmt.Printf("Gatter %d: Zwischenergebnis = %s, Carry = %s\n", i+1, sum, carry)
    }
}