package emu

import (
	"errors"
	"github.com/tedkotz/trich-ternary/core"
)

// RegisterBank manages the state of all system registers.
type RegisterBank struct {
	data [8]core.Trit
}

// Write sets the value of a specific register.
func (rb *RegisterBank) Write(index int, val core.Trit) error {
	if index < 0 || index >= 8 {
		return errors.New("register index out of bounds")
	}
	rb.data[index] = val
	return nil
}

// Read returns the value of a specific register.
func (rb *RegisterBank) Read(index int) (core.Trit, error) {
	if index < 0 || index >= 8 {
		return core.Zero, errors.New("register index out of bounds")
	}
	return rb.data[index], nil
}
