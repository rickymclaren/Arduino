/*
   ESP32-2432S028 (ST7789) - Hello CYD
   Simple test for the included 2.4" ST7789 TFT
*/

#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  
  // Initialize ST7789 display
  tft.init();
  
  // Set rotation (1 = landscape, most common for this board)
  tft.setRotation(1);
  
  // Enable backlight (pin 21 on most CYD boards)
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);
  
  // Clear screen to black
  tft.fillScreen(TFT_BLACK);
  
  // Center alignment
  tft.setTextDatum(MC_DATUM);
  
  // Big "Hello"
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(4);
  tft.drawString("Hello", tft.width()/2, 80);
  
  // "CYD" in cyan, even bigger
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(6);
  tft.drawString("CYD", tft.width()/2, 155);
  
  // Info line
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("ESP32-2432S028", tft.width()/2, 235);
  
  // Decorative border
  tft.drawRect(8, 8, tft.width()-16, tft.height()-16, TFT_MAGENTA);
  tft.drawRect(12, 12, tft.width()-24, tft.height()-24, TFT_GREEN);
}

void loop() {
  // Optional: Slowly pulse the "CYD" color for a nice effect
  static uint8_t hue = 0;
  hue++;
  
  uint16_t color = tft.color565(0, hue*2, 255);  // Nice cyan to blue shift
  tft.setTextColor(color);
  tft.setTextSize(6);
  tft.drawString("CYD", tft.width()/2, 155);
  
  delay(80);
}