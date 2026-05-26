package core

// TernaryNAND ist das Fundament (Min-NAND)
func TernaryNAND(a, b Trit) Trit {
    min := a
    if b < min { min = b }
    return -min
}

func TernaryNOT(a Trit) Trit { return -a }

// TernaryAND: NAND mit Invertierung
func TernaryAND(a, b Trit) Trit {
    return TernaryNOT(TernaryNAND(a, b))
}

// TernaryOR: Basierend auf De Morgan's Laws für Ternär-Logik
func TernaryOR(a, b Trit) Trit {
    return TernaryNAND(TernaryNOT(a), TernaryNOT(b))
}

// TernaryXOR: Wichtig für Addition! 
// XOR ist hier etwas spezieller im Ternär-System.
func TernaryXOR(a, b Trit) Trit {
    if a == b { return Zero }
    return PosOne
}