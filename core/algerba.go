package core

// FullAdder berechnet die Summe von zwei Trits und berücksichtigt den Carry.
// Das ist der "heiligste" Block in deiner Architektur.
func FullAdder(a, b, carryIn Trit) (sum, carryOut Trit) {
    // Sum = (a + b + carryIn) mod 1
    sum = TernarySum(TernarySum(a, b), carryIn)
    
    // Carry-Logik: Ein Carry tritt auf, wenn der Wert das Intervall [-1, 1] verlässt
    carryOut = CalculateCarry(a, b, carryIn)
    return sum, carryOut
}

func TernarySum(a, b Trit) Trit {
    val := a.ToInt() + b.ToInt()
    if val > 1 { return NegOne }
    if val < -1 { return PosOne }
    return Trit(val)
}

func CalculateCarry(a, b, c Trit) Trit {
    // Einfache Majority-Logik für den Carry
    if a == b || a == c || b == c {
        return a // Vereinfachte Carry-Logik für Balanced Ternary
    }
    return Zero
}