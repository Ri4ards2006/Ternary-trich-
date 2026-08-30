package core

import "testing"

func TestAlgebra(t *testing.T) {
    // Test: 1 + 1 + 1 (PosOne + PosOne + PosOne) = 0 mit Carry +
    s, c := FullAdder(PosOne, PosOne, PosOne)
    if s != Zero || c != PosOne {
        t.Errorf("FullAdder failed: expected 0 with carry +, got %v with carry %v", s, c)
    }
}  