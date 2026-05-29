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

// Das hier hat gefehlt
func performCalculation(c *emu.CPU, count int) {
	c.State.Registers[1] = core.PosOne
	c.State.Registers[2] = core.PosOne
	
	// Dynamische Ausführung
	for i := 0; i < count; i++ {
		c.Add(0, 1, 2)
	}
	fmt.Println("Berechnung abgeschlossen.")
}