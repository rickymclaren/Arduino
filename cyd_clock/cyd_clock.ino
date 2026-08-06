/*
   ESP32-2432S028 (ILI9341) WiFi Clock
   Built-in TFT Display
*/

#define LV_CONF_INCLUDE_SIMPLE

#include <LVGL_CYD.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <arduino_secrets.h>
#include "ui.h"

// Custom I2C pins
#define SDA_PIN 27
#define SCL_PIN 22

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

void setup() {

  // Start Serial and lvgl, set everything up for this device
  // Will also do Serial.begin(115200), lvgl.init(), set up the touch driver
  // and the lvgl timer.
  LVGL_CYD::begin(USB_LEFT);
  LVGL_CYD::backlight(255);

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
  
  ui_init();

  static lv_point_precise_t line_points[] = { 
    {55, 90}, {65, 90}, 
    {55, 100}, {65, 100}, 
    {55, 110}, {65, 110}, 
    {55, 120}, {65, 120}, 
    {55, 130}, {70, 130}, 
    {55, 140}, {65, 140}, 
    {55, 150}, {65, 150}, 
    {55, 160}, {65, 160}, 
    {55, 170}, {65, 170}, 
  };

  /*Create style*/
  static lv_style_t style_line;
  lv_style_init(&style_line);
  lv_style_set_line_width(&style_line, 2);
  lv_style_set_line_color(&style_line, lv_color_hex(0x4f4f4f));
  lv_style_set_line_rounded(&style_line, false);

  /*Create a line and apply the new style*/
  int lines = sizeof(line_points) / sizeof(line_points[0]) / 2;
  for (int i=0; i<lines; i++) {
    lv_obj_t * line1;
    line1 = lv_line_create(lv_scr_act());
    lv_line_set_points(line1, &line_points[i*2], 2);     /*Set the points*/
    lv_obj_add_style(line1, &style_line, 0);
  }

  // Create update timer
  lv_timer_create(update_display, 1000, NULL);

}

void update_display(lv_timer_t * timer) {
    Serial.println("Updating");

    timeClient.update();
    
    String time_str = timeClient.getFormattedTime(); // HH:MM:SS
    
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    float temperature = temp.temperature;
    float hum = humidity.relative_humidity;
    float pressure = bmp.readPressure() / 100.0F;

    Serial.print("Time ");
    Serial.println(time_str);
    lv_label_set_text(ui_time, time_str.c_str());

    String date_str = getDateString();
    Serial.println(date_str);
    lv_label_set_text(ui_date, date_str.c_str());

    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "%.1f °C", temperature);
    Serial.print("Temp: ");
    Serial.println(temp_str);
    lv_label_set_text(ui_temp, temp_str);
    lv_bar_set_value(ui_BarTemp, (int) (temperature * 10), LV_ANIM_OFF);

    char hum_str[32];
    snprintf(hum_str, sizeof(hum_str), "%.1f %%", hum);
    Serial.print("Hum: ");
    Serial.println(hum_str);
    lv_label_set_text(ui_hum, hum_str);
    lv_arc_set_value(ui_ArcHumidity, (int) hum);

    char press_str[32];
    snprintf(press_str, sizeof(press_str), "%.0f hPa", pressure);
    Serial.print("Press: ");
    Serial.println(press_str);
    lv_label_set_text(ui_press, press_str);
    lv_arc_set_value(ui_ArcPressure, (int) pressure);
}

void loop() {
    // lvgl needs this to be called in a loop to run the interface
  lv_task_handler();
  delay(5);

}

String getDateString() {
  time_t rawTime = timeClient.getEpochTime();
  struct tm *timeinfo = localtime(&rawTime);
  
  char buffer[40];
  strftime(buffer, sizeof(buffer), "  %A, %B %d %Y  ", timeinfo);
  return String(buffer);
}


