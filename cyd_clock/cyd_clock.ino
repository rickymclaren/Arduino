/*
   ESP32-2432S028 (ILI9341) WiFi Clock
   Built-in TFT Display
*/

#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>
#include <arduino_secrets.h>

// ================== CONFIG ==================
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// Timezone offset in seconds (e.g. GMT+1 = 3600)
const long utcOffsetInSeconds = 3600;   

// ===========================================

TFT_eSPI tft = TFT_eSPI();
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 60000);

unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize the ILI9341 display
  tft.init();
  tft.setRotation(3);           // 1 = Landscape (most common for this board)
  tft.fillScreen(TFT_BLACK);
  
  // Backlight ON (usually pin 21 on this board)
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);   // Center alignment

  tft.setTextSize(2);
  tft.drawString("Connecting to WiFi...", tft.width()/2, tft.height()/2 - 20);
  
  // Connect WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  timeClient.begin();
  
  tft.fillScreen(TFT_BLACK);
  tft.drawString("WiFi Connected!", tft.width()/2, 100);
  delay(1500);
}

void loop() {
  if (millis() - lastUpdate >= 1000) {
    timeClient.update();
    
    String timeStr = timeClient.getFormattedTime().substring(0, 5); // HH:MM
    String secStr   = timeClient.getFormattedTime().substring(6, 8);
    
    tft.fillScreen(TFT_BLACK);
    
    // Big Time
    tft.setTextSize(5);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(timeStr, tft.width()/2, 85);
    
    // Seconds
    tft.setTextSize(3);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(secStr, tft.width()/2 + 115, 105);
    
    // Date
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(getDateString(), tft.width()/2, 200);
    
    // Status
    tft.setTextSize(1);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("ESP32-2432S028  |  NTP Time", tft.width()/2, 290);
    
    lastUpdate = millis();
  }
  
  delay(10);
}

String getDateString() {
  time_t rawTime = timeClient.getEpochTime();
  struct tm *timeinfo = localtime(&rawTime);
  
  char buffer[40];
  strftime(buffer, sizeof(buffer), "%A, %B %d %Y", timeinfo);
  return String(buffer);
}