package core

// TernaryNOR ist eine Funktion, die deine Grundgatter nutzt.
// Du stapelst also bestehende Funktionen, um neue zu bauen.
func TernaryNOR(a, b Trit) Trit {
    // Beispiel: NOR ist invertiertes OR
    // Du kannst hier einfach deine existierenden Funktionen kombinieren
    return TernaryNOT(TernaryOR(a, b))
} 

// Beispiel: Dein "Alias" System
func TernaryOR(a, b Trit) Trit {
    // Implementiere die Logik für OR
    if a == PosOne || b == PosOne {
        return PosOne
    }
    return TernaryNAND(a, b) // Oder wie auch immer du es definieren willst
}

func TernaryNOT(a Trit) Trit {
    return -a
} a