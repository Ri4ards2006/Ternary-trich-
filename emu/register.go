package emu

import (
    "github.com/tedkotz/trich-ternary/core"
)

type RegisterBank struct {
    Data [][]core.Trit // Ein Slice von Wörtern
}

func NewRegisterBank(numRegs, wordSize int) *RegisterBank {
    data := make([][]core.Trit, numRegs)
    for i := range data {
        data[i] = make([]core.Trit, wordSize)
    }
    return &RegisterBank{Data: data}
}