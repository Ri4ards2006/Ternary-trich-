package core



// Trit repräsentiert einen der drei Zustände: {-1, 0, 1}
type Trit int8  // i intentially used the int8 so no fucking body can write fa: 42


const (
    NegOne Trit = -1
    Zero   Trit = 0
    PosOne Trit = 1
)

// TernaryNAND implementiert ein universelles Gatter für Balanced Ternary.
// Wir nutzen hier die Wahrheitstabelle für das sogenannte 'Min-NAND'.
func TernaryNAND(a, b Trit) Trit {
    // Logik: Invertiertes Minimum der Eingänge
    // Dies ist eine der theoretisch saubersten Definitionen für Ternär-Gatter.
    min := a
    if b < min {
        min = b
    }
    
    // Invertierung in Balanced Ternary: -1 wird zu 1, 0 bleibt 0, 1 wird zu -1
    return -min
}

// String konvertiert den Trit in eine lesbare Form für deine Logs
func (t Trit) String() string {
    switch t {
    case NegOne: return "-"
    case Zero:   return "0"
    case PosOne: return "+"
    default:     return "?"
    }
}