/*
   ESP32-2432S028 (ILI9341) WiFi Clock
   Built-in TFT Display
*/

#define LV_CONF_INCLUDE_SIMPLE

#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <Matter.h>
#include <arduino_secrets.h>

// Custom I2C pins
#define SDA_PIN 22
#define SCL_PIN 23

// ================== CONFIG ==================
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// Timezone offset in seconds (e.g. GMT+1 = 3600)
const long utcOffsetInSeconds = 3600;   

// ===========================================

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 60000);

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

MatterTemperatureSensor mT;
MatterHumiditySensor mH;
MatterPressureSensor mP;

void setup() {

  Serial.begin(15200);
  delay(1000);
  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize Sensors
  if (!aht.begin()) {
    Serial.println("AHT20 not found!");
  }
  if (!bmp.begin(0x77)) {       // Change to 0x77 if needed
    Serial.println("BMP280 not found!");
  }

  // Connect WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  timeClient.begin();

  mT.begin();
  mP.begin();
  mH.begin();
  Matter.begin();

  if (!Matter.isDeviceCommissioned()) {
    Serial.println("Matter device not commissioned ...");
    Serial.printf("Manual pairing code: %s\n", Matter.getManualPairingCode().c_str());
    Serial.printf("QR code URL: %s\n", Matter.getOnboardingQRCodeUrl().c_str());
    Serial.println("Waiting for Matter commisioning");
  }

  while (!Matter.isDeviceCommissioned()) {
    delay(5000);
  }

  Serial.println("Matter device commisioned successfully");
  
}

void loop() {
    Serial.println("Updating");

    timeClient.update();
    
    String time_str = timeClient.getFormattedTime(); // HH:MM:SS
    
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    float temperature = temp.temperature;
    float hum = humidity.relative_humidity;
    float pressure = round(bmp.readPressure() / 100.0F);

    mT.setTemperature(temperature);
    mP.setPressure(pressure);
    mH.setHumidity(hum);

    Serial.print("Time ");
    Serial.println(time_str);

    String date_str = getDateString();
    Serial.println(date_str);

    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "%.1f °C", temperature);
    Serial.print("Temp: ");
    Serial.println(temp_str);

    char hum_str[32];
    snprintf(hum_str, sizeof(hum_str), "%.1f %%", hum);
    Serial.print("Hum: ");
    Serial.println(hum_str);

    char press_str[32];
    snprintf(press_str, sizeof(press_str), "%.0f hPa", pressure);
    Serial.print("Press: ");
    Serial.println(press_str);

    delay(1000);
}

String getDateString() {
  time_t rawTime = timeClient.getEpochTime();
  struct tm *timeinfo = localtime(&rawTime);
  
  char buffer[40];
  strftime(buffer, sizeof(buffer), "  %A, %B %d %Y  ", timeinfo);
  return String(buffer);
}


