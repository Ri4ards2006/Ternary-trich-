package emu

import (
	"errors"
	"github.com/tedkotz/trich-ternary/core"
)

// Memory represents the system RAM.
type Memory struct {
	cells []core.Trit
}

// NewMemory allocates a new memory block of a specific size.
func NewMemory(size int) *Memory {
	return &Memory{
		cells: make([]core.Trit, size),
	}
}

// Load reads a value from a specific address.
func (m *Memory) Load(address int) (core.Trit, error) {
	if address < 0 || address >= len(m.cells) {
		return core.Zero, errors.New("memory access violation: address out of bounds")
	}
	return m.cells[address], nil
}

// Store writes a value to a specific address.
func (m *Memory) Store(address int, val core.Trit) error {
	if address < 0 || address >= len(m.cells) {
		return errors.New("memory access violation: address out of bounds")
	}
	m.cells[address] = val
	return nil
}

// Size returns the total capacity of the memory.
func (m *Memory) Size() int {
	return len(m.cells)
} 
