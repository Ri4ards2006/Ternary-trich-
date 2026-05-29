package core

func FullAdder(a, b, carryIn Trit) (sum, carryOut Trit) {
    val := a.ToInt() + b.ToInt() + carryIn.ToInt()
    
    // Balanced Ternary Logik
    switch val {
    case 3:  return Zero, PosOne
    case 2:  return NegOne, PosOne
    case 1:  return PosOne, Zero
    case 0:  return Zero, Zero
    case -1: return NegOne, Zero
    case -2: return PosOne, NegOne
    case -3: return Zero, NegOne
    default: return Zero, Zero
    }
}