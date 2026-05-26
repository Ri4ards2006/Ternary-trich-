package core
type Trit int8
const ( NegOne Trit = -1; Zero Trit = 0; PosOne Trit = 1 )
func (t Trit) String() string { /* ... dein Switch-Case ... */ }

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