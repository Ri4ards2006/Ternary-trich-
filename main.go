package main

import (
	"fmt"
	"github.com/tedkotz/trich-ternary/core"
	"github.com/tedkotz/trich-ternary/emu"
)

func main() {
    fmt.Println("--- Ternary Hardware Configurator ---")
    
    // 1. Frage ab: Wie viele Gatter/Operationen?
    fmt.Print("Wie viele Additions-Gatter willst du kaskadieren? ")
    var gateCount int
    fmt.Scan(&gateCount)

    // 2. Erstelle eine dynamische ALU basierend auf der Wahl
    // Du kannst hier eine Liste oder Map füllen
    fmt.Println("Konfiguriere", gateCount, "Gatter...")
    
    // 3. Main Loop für die Interaktion
    for {
        fmt.Println("\n--- Steuerkonsole ---")
        fmt.Println("1: Rechnung ausführen")
        fmt.Println("2: Hardware umkonfigurieren")
        fmt.Println("3: Exit")
        
        var wahl int
        fmt.Scan(&wahl)
        
        switch wahl {
        case 1:
            // Hier triggerst du den Rechenzyklus
            performCalculation(gateCount)
        case 2:
            // Hier gehst du zurück zur Abfrage
            main() 
        case 3:
            return
        }
    }
}