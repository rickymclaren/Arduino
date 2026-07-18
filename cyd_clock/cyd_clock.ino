/*
   ESP32-2432S028 (ILI9341) WiFi Clock
   Built-in TFT Display
*/

#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <arduino_secrets.h>

// Custom I2C pins
#define SDA_PIN 27
#define SCL_PIN 22

#define BACKLIGHT_PIN 27

// ================== CONFIG ==================
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// Timezone offset in seconds (e.g. GMT+1 = 3600)
const long utcOffsetInSeconds = 3600;   

// ===========================================

TFT_eSPI tft = TFT_eSPI();
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 60000);

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

// Variables for flicker reduction
String lastTime = "";
float lastTemp = -999;
float lastHum = -999;
float lastPress = -999;
String lastDate = "";
bool firstDraw = true;

unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize the ILI9341 display
  tft.init();
  tft.setRotation(3);           // 1 = Landscape (most common for this board)
  tft.fillScreen(TFT_BLACK);
  
  // Backlight ON (usually pin 21 on this board)
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);   // Center alignment

  // Initialize Sensors
  if (!aht.begin()) Serial.println("AHT20 not found!");
  if (!bmp.begin(0x77)) {       // Change to 0x77 if needed
    Serial.println("BMP280 not found!");
  }

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
    
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    float temperature = temp.temperature;
    float hum = humidity.relative_humidity;
    float pressure = bmp.readPressure() / 100.0F;

    int w = tft.width();   // 320
    int colW = w / 3;

    if (firstDraw) {
      tft.fillScreen(TFT_BLACK);
      drawThermometer(colW/2 - 12, 60);
      drawDroplet(colW + colW/2 - 12, 60);
      drawPressureIcon(2*colW + colW/2 - 15, 60);
    }
    
    // Big Time
    tft.setTextSize(5);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(timeStr, tft.width()/2, 35);
    
    // Seconds
    tft.setTextSize(3);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(secStr, tft.width()/2 + 115, 35);
    
    // Date
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(getDateString(), tft.width()/2, 210);
    
    // Update Temperature
    if (firstDraw || abs(temperature - lastTemp) > 0.1) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setTextSize(3);
      tft.drawString(String(temperature, 1), colW/2, 150);
      tft.setTextSize(2);
      tft.drawString("C", 1*colW/2, 180);
      lastTemp = temperature;
    }

    // Update Humidity
    if (firstDraw || abs(hum - lastHum) > 0.2) {
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setTextSize(3);
      tft.drawString(String(hum, 0), 2*colW/2 + colW/2, 150);
      tft.setTextSize(2);
      tft.drawString("%", 3*colW/2, 180);
      lastHum = hum;
    }

    // Update Pressure
    if (firstDraw || abs(pressure - lastPress) > 0.3) {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextSize(3);
      tft.drawString(String(pressure, 0), 5*colW/2, 150);
      tft.setTextSize(2);
      tft.drawString("hPa", 5*colW/2, 180);
      lastPress = pressure;
    }

    firstDraw = false;
    lastUpdate = millis();
  }
  
  delay(10);
}

String getDateString() {
  time_t rawTime = timeClient.getEpochTime();
  struct tm *timeinfo = localtime(&rawTime);
  
  char buffer[40];
  strftime(buffer, sizeof(buffer), "  %A, %B %d %Y  ", timeinfo);
  return String(buffer);
}

// ===================== ICONS =====================

void drawThermometer(int x, int y) {
  tft.fillRect(x + 8, y, 14, 48, TFT_WHITE);           // Tube
  tft.fillRect(x + 10, y + 4, 10, 38, TFT_BLACK);
  tft.fillCircle(x + 15, y + 52, 14, TFT_RED);         // Bulb
  tft.fillCircle(x + 11, y + 48, 3, TFT_PINK);
  
  // Scale ticks
  for (int i = 0; i < 4; i++) {
    tft.drawFastHLine(x + 22, y + 10 + i*8, 10, TFT_WHITE);
  }
}

void drawDroplet(int x, int y) {
  tft.fillCircle(x + 15, y + 20, 17, TFT_CYAN);
  tft.fillTriangle(x - 2, y + 24, x + 32, y + 24, x + 15, y + 48, TFT_CYAN);
  tft.fillCircle(x + 13, y + 17, 9, TFT_WHITE);   // Highlight
}

void drawPressureIcon(int x, int y) {
  tft.drawCircle(x + 20, y + 25, 24, TFT_YELLOW);  // Outer
  tft.drawCircle(x + 20, y + 25, 16, TFT_YELLOW);  // Inner
  
  // Needle
  tft.drawLine(x + 20, y + 25, x + 35, y + 13, TFT_YELLOW);
  
  // Ticks
  for (int i = -30; i <= 30; i += 15) {
    float rad = i * PI / 180.0;
    tft.drawLine(x + 20 + 20*cos(rad), y + 25 + 20*sin(rad),
                 x + 20 + 27*cos(rad), y + 25 + 27*sin(rad), TFT_YELLOW);
  }
}