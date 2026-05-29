package core

// ALU ist jetzt ein Interface. Du kannst verschiedene Versionen bauen.
type ALU interface {
    Compute(a, b, carryIn Trit) (Trit, Trit)
}

// StandardALU nutzt deine jetzigen Gatter.
type StandardALU struct{}

func (s *StandardALU) Compute(a, b, carryIn Trit) (Trit, Trit) {
    return FullAdder(a, b, carryIn)
}