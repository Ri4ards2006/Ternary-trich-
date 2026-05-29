package core

func FullAdder(a, b, carryIn Trit) (sum, carryOut Trit) {
    sum = TernarySum(TernarySum(a, b), carryIn)
    carryOut = CalculateCarry(a, b, carryIn)
    return sum, carryOut
}

func TernarySum(a, b Trit) Trit {
    // Hier greifen wir jetzt auf die Methode aus types.go zu
    val := a.ToInt() + b.ToInt()
    
    // In Balanced Ternary: -1 + -1 = -2 -> +1 (mit Carry -1)
    // Das ist die Logik, die du brauchst:
    if val > 1 { return NegOne }
    if val < -1 { return PosOne }
    return Trit(val)
}

func CalculateCarry(a, b, c Trit) Trit {
    val := a.ToInt() + b.ToInt() + c.ToInt()
    if val > 1 { return PosOne }
    if val < -1 { return NegOne }
    return Zero
}