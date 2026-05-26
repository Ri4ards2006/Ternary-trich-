package core

import "testing"

func TestTernaryNAND(t *testing.T) {
    // Wir testen: NAND(+, +) sollte - ergeben
    result := TernaryNAND(PosOne, PosOne)
    if result != NegOne {
        t.Errorf("Test failed: expected NegOne (-), got %v", result)
    }

    // Wir testen: NAND(0, -) sollte + ergeben
    result = TernaryNAND(Zero, NegOne)
    if result != PosOne {
        t.Errorf("Test failed: expected PosOne (+), got %v", result)
    }
}