package core

// TernaryNAND ist das universelle Gatter für Ternär-Logik (Setun-Style)
func TernaryNAND(a, b Trit) Trit {
	// Min(a, b) invertiert
	min := a
	if b < min { min = b }
	return -min
}

// TernaryNOT ist die Invertierung: + -> -, - -> +, 0 -> 0
func TernaryNOT(a Trit) Trit {
	return -a
}

// TernaryOR implementiert eine Disjunktion für das ternäre System
func TernaryOR(a, b Trit) Trit {
	if a == PosOne || b == PosOne { return PosOne }
	if a == NegOne && b == NegOne { return NegOne }
	return Zero
}