package emu

import "github.com/tedkotz/trich-ternary/core"

type ALU struct{}

func (a *ALU) Compute(op string, x, y []core.Trit) []core.Trit {
    result := make([]core.Trit, len(x))
    switch op {
    case "ADD":
        // Hier kommt später deine Loop-Logik rein
        return result 
    default:
        return x
    }
}