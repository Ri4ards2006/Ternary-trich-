package core

type Trit int8

const (
    NegOne Trit = -1
    Zero   Trit = 0
    PosOne Trit = 1
)

// Ein Word ist ein Slice aus Trits
type Word []Trit 

func (t Trit) String() string {
    switch t {
    case NegOne: return "-"
    case Zero:   return "0"
    case PosOne: return "+"
    default:     return "?"
    }
}