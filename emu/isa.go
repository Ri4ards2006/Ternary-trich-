package emu

// Jetzt MUSS das funktionieren, weil Add im Paket emu definiert ist
var OpCodes = map[string]func(*CPU, int, int, int){
    "ADD": (*CPU).Add,
    "SUB": (*CPU).Sub,
}