package emu

import "github.com/tedkotz/trich-ternary/core"

type ALU struct{}

func (a *ALU) Compute(op string, x, y []core.Trit) []core.Trit {
    switch op {
    case "ADD":
        return a.Add(x, y) // Deine bestehende Add-Logik für Wörter
    case "SUB":
        return a.Sub(x, y)
    default:
        return x
    }
}