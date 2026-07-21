/*
   ESP32-2432S028 (ILI9341) WiFi Clock
   Built-in TFT Display
*/

#define LV_CONF_INCLUDE_SIMPLE

#include <string.h>
#include <LVGL_CYD.h>
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

// LVGL objects
static lv_obj_t * time_label;
static lv_obj_t * date_label;
static lv_obj_t * temp_label;
static lv_obj_t * hum_label;
static lv_obj_t * press_label;

// SVG Data for Humidity Icon (Water Drop) - Embedded SVG string
// You can replace this with a more complex SVG
const char humidity_svg[] = 
"<svg width='200' height='220' viewBox='0 0 200 220' xmlns='http://www.w3.org/2000/svg'>"
"  <defs>"
"    <linearGradient id='waterGrad' x1='50%' y1='20%' x2='50%' y2='100%'>"
"      <stop offset='0%' stop-color='#81E4FF'/>"
"      <stop offset='100%' stop-color='#4FC3F7'/>"
"    </linearGradient>"
"  </defs>"
"  <!-- Main water drop -->"
"  <path d='M100 30 Q50 120 50 160 Q50 190 80 205 Q100 215 120 205 Q150 190 150 160 Q150 120 100 30 Z' "
"        fill='url(#waterGrad)' stroke='#0288D1' stroke-width='12'/>"
"  <!-- Inner highlight -->"
"  <path d='M100 45 Q65 115 72 155 Q80 170 95 175' fill='none' stroke='#E0F7FA' stroke-width='18' opacity='0.75'/>"
"  <!-- Ripples -->"
"  <ellipse cx='100' cy='135' rx='32' ry='12' fill='none' stroke='#E0F7FA' stroke-width='6' opacity='0.7'/>"
"  <ellipse cx='100' cy='165' rx='24' ry='9' fill='none' stroke='#E0F7FA' stroke-width='5' opacity='0.6'/>"
"</svg>";

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
  
  // Start Serial and lvgl, set everything up for this device
  // Will also do Serial.begin(115200), lvgl.init(), set up the touch driver
  // and the lvgl timer.
  LVGL_CYD::begin(USB_LEFT);
  LVGL_CYD::backlight(255);

  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // Backlight ON (usually pin 21 on this board)
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  // Initialize Sensors
  if (!aht.begin()) Serial.println("AHT20 not found!");
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
  
  lv_obj_t * screen = lv_scr_act();
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x112233), LV_PART_MAIN);

  // Time label
  time_label = lv_label_create(screen);
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 20);
  lv_label_set_text(time_label, "time_str");

  // Date label
  date_label = lv_label_create(screen);
  lv_obj_set_style_text_font(date_label, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(date_label, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(date_label, LV_ALIGN_TOP_MID, 0, 60);
  lv_label_set_text(date_label, "date_str");

  temp_label = lv_label_create(screen);
  lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(temp_label, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(temp_label, LV_ALIGN_LEFT_MID, 20, 10);
  lv_label_set_text(temp_label, "temp_str");

  hum_label = lv_label_create(screen);
  lv_obj_set_style_text_font(hum_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(hum_label, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(hum_label, LV_ALIGN_LEFT_MID, 20, 40);
  lv_label_set_text(hum_label, "hum_str");

  press_label = lv_label_create(screen);
  lv_obj_set_style_text_font(press_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(press_label, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(press_label, LV_ALIGN_LEFT_MID, 20, 70);
  lv_label_set_text(press_label, "press_str");

  // Create image object and set SVG source (ThorVG handles parsing & rendering)
  lv_obj_t *img = lv_img_create(screen);
  lv_obj_align(img, LV_ALIGN_CENTER, 0, -20);
  
  lv_img_set_src(img, humidity_svg);   // ThorVG decodes this SVG

  // Optional: Scale the image
  lv_img_set_zoom(img, 280);           // 280% zoom (adjust for your display)
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
    lv_label_set_text(time_label, time_str.c_str());

    String date_str = getDateString();
    Serial.println(date_str);
    lv_label_set_text(date_label, date_str.c_str());

    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "Temp: %.1f °C", temperature);
    Serial.print("Temp: ");
    Serial.println(temp_str);
    lv_label_set_text(temp_label, temp_str);

    char hum_str[32];
    snprintf(hum_str, sizeof(hum_str), "Hum: %.1f %%", hum);
    Serial.print("Hum: ");
    Serial.println(hum_str);
    lv_label_set_text(hum_label, hum_str);

    char press_str[32];
    snprintf(press_str, sizeof(press_str), "Pressure: %.0f hPa", pressure);
    Serial.print("Press: ");
    Serial.println(press_str);
    lv_label_set_text(press_label, press_str);
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
