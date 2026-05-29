#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

void setup() {
    uint16_t ID = tft.readID(); // Identifiziert deinen Display-Chip
    tft.begin(ID);
    tft.setRotation(1); // Querformat
    tft.fillScreen(0x0000); // Schwarz machen
    tft.setCursor(0, 0);
    tft.setTextColor(0xFFFF); // Weißer Text
    tft.print("Ternaryrator Online");
}

void loop() {
    // Hier passiert später die Ternär-Logik
}  