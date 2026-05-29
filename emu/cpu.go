// In emu/cpu.go

package emu

import (
    "fmt"
    "github.com/tedkotz/trich-ternary/core"
)
// ADD existiert ja schon bei dir
func (c *CPU) Add(target, src1, src2 int) {
    // ... dein Add Code ...
}

// DAS HIER FEHLT DIR (Deswegen meckert der Compiler):
// ... nach deiner Add-Funktion ...
func (c *CPU) Sub(target, src1, src2 int) {
    // Hier die Subtraktion
    negB := core.Negate(c.State.Registers[src2]) // Annahme: core hat Negate
    sum, carry := core.FullAdder(c.State.Registers[src1], negB, core.Zero)
    c.State.Registers[target] = sum
    c.State.CarryFlag = carry
    fmt.Printf("Executed SUB: R%d = R%d - R%d\n", target, src1, src2)
}