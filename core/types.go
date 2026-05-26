package core  // The Core folder with all the Go Stuff

import "fmt"  

// Trit repräsentiert die 3 Zustände: {-1, 0, 1}
type Trit int8

const (
	NegOne Trit = -1
	Zero   Trit = 0
	PosOne Trit = 1
)

// ToInt konvertiert Trit zu Integer für einfache Berechnungen
func (t Trit) ToInt() int {
	return int(t)
}

// FromInt konvertiert Integer zu Trit (sicherheitsorientiert)
func FromInt(i int) Trit {
	switch i {
	case -1: return NegOne
	case 1:  return PosOne
	default: return Zero
	}
}

func (t Trit) String() string {
	switch t {
	case NegOne: return "-"
	case Zero:   return "0"
	case PosOne: return "+"
	default:     return "?"
	}
}