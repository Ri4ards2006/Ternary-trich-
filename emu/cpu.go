package emu

import "github.com/tedkotz/trich-ternary/core"

type CPU struct {
    State  *CPUState
    Memory *Memory
    ALU    core.ALU // Hier ist die "Beschaltung"
}

func NewCPU(memSize int, alu core.ALU) *CPU {
    return &CPU{
        State:  &CPUState{},
        Memory: NewMemory(memSize),
        ALU:    alu, // Jetzt kannst du beim Start sagen, welche ALU du willst
    }
}