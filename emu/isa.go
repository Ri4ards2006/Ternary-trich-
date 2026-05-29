package emu

// Hier definieren wir die Befehls-Tabelle
var OpCodes = map[string]func(*CPU, int, int, int){
    "ADD": (*CPU).Add,
    "SUB": (*CPU).Sub,
}