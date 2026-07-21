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
  lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_LEFT, 0); 
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
  lv_obj_align(temp_label, LV_ALIGN_LEFT_MID, 20, 100);
  lv_label_set_text(temp_label, "temp_str");

  hum_label = lv_label_create(screen);
  lv_obj_set_style_text_font(hum_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(hum_label, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(hum_label, LV_ALIGN_LEFT_MID, 130, 100);
  lv_label_set_text(hum_label, "hum_str");

  press_label = lv_label_create(screen);
  lv_obj_set_style_text_font(press_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(press_label, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(press_label, LV_ALIGN_LEFT_MID, 230, 100);
  lv_label_set_text(press_label, "press_str");

// Create icons in a row
  create_temp_icon(screen, -110, 30);
  create_humidity_icon(screen, 0, 25);
  create_pressure_icon(screen, 105, 25);  

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
    snprintf(temp_str, sizeof(temp_str), "%.1f °C", temperature);
    Serial.print("Temp: ");
    Serial.println(temp_str);
    lv_label_set_text(temp_label, temp_str);

    char hum_str[32];
    snprintf(hum_str, sizeof(hum_str), "%.1f %%", hum);
    Serial.print("Hum: ");
    Serial.println(hum_str);
    lv_label_set_text(hum_label, hum_str);

    char press_str[32];
    snprintf(press_str, sizeof(press_str), "%.0f hPa", pressure);
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

// Temperature Icon (Thermometer)
void create_temp_icon(lv_obj_t *parent, lv_coord_t x, lv_coord_t y) {
  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_size(cont, 82, 120);
  lv_obj_align(cont, LV_ALIGN_CENTER, x, y);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

  // Glass tube
  lv_obj_t *tube = lv_obj_create(cont);
  lv_obj_set_size(tube, 26, 78);
  lv_obj_align(tube, LV_ALIGN_BOTTOM_MID, 0, -12);
  lv_obj_set_style_bg_color(tube, lv_color_hex(0xFF5252), 0);
  lv_obj_set_style_radius(tube, 13, 0);
  lv_obj_set_style_border_width(tube, 8, 0);
  lv_obj_set_style_border_color(tube, lv_color_hex(0xFF8A65), 0);

  // Red bulb
  lv_obj_t *bulb = lv_obj_create(cont);
  lv_obj_set_size(bulb, 38, 38);
  lv_obj_align(bulb, LV_ALIGN_BOTTOM_MID, 0, 2);
  lv_obj_set_style_bg_color(bulb, lv_color_hex(0xFF1744), 0);
  lv_obj_set_style_radius(bulb, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(bulb, 6, 0);
  lv_obj_set_style_border_color(bulb, lv_color_hex(0xFF8A65), 0);

  // Temperature marks
  for (int i = 0; i < 4; i++) {
    lv_obj_t *mark = lv_obj_create(cont);
    lv_obj_set_size(mark, 10, 3);
    lv_obj_align(mark, LV_ALIGN_TOP_MID, 18, 15 + i * 13);
    lv_obj_set_style_bg_color(mark, lv_color_white(), 0);
  }

}

// Humidity Icon (Water Drop - pure vector)
void create_humidity_icon(lv_obj_t *parent, lv_coord_t x, lv_coord_t y) {
  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_size(cont, 90, 110);
  lv_obj_align(cont, LV_ALIGN_CENTER, x, y);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

  // Main drop
  lv_obj_t *drop = lv_obj_create(cont);
  lv_obj_set_size(drop, 68, 82);
  lv_obj_align(drop, LV_ALIGN_CENTER, 0, 8);
  lv_obj_set_style_bg_color(drop, lv_color_hex(0x40C4FF), 0);
  lv_obj_set_style_radius(drop, 60, 0);           // Makes it teardrop-like
  lv_obj_set_style_border_width(drop, 10, 0);
  lv_obj_set_style_border_color(drop, lv_color_hex(0x0277BD), 0);
  lv_obj_clear_flag(drop, LV_OBJ_FLAG_SCROLLABLE);

  // Top point cover (makes it pointed)
  // lv_obj_t *top = lv_obj_create(cont);
  // lv_obj_set_size(top, 52, 38);
  // lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 12);
  // lv_obj_set_style_bg_color(top, lv_color_hex(0x40C4FF), 0);
  // lv_obj_set_style_border_opa(top, LV_OPA_TRANSP, 0);
  // lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

  // Highlight
  lv_obj_t *highlight = lv_obj_create(cont);
  lv_obj_set_size(highlight, 12, 16);
  lv_obj_align(highlight, LV_ALIGN_TOP_MID, -12, 28);
  lv_obj_set_style_bg_color(highlight, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(highlight, 90, 0);
  lv_obj_set_style_radius(highlight, 20, 0);
  lv_obj_clear_flag(highlight, LV_OBJ_FLAG_SCROLLABLE);

  // Ripples
  // lv_obj_t *r1 = lv_obj_create(cont);
  // lv_obj_set_size(r1, 42, 8);
  // lv_obj_align(r1, LV_ALIGN_CENTER, 0, 28);
  // lv_obj_set_style_bg_opa(r1, LV_OPA_TRANSP, 0);
  // lv_obj_set_style_border_width(r1, 4, 0);
  // lv_obj_set_style_border_color(r1, lv_color_white(), 0);
  // lv_obj_set_style_radius(r1, LV_RADIUS_CIRCLE, 0);
  // lv_obj_clear_flag(r1, LV_OBJ_FLAG_SCROLLABLE);

  // lv_obj_t *r2 = lv_obj_create(cont);
  // lv_obj_set_size(r2, 28, 6);
  // lv_obj_align(r2, LV_ALIGN_CENTER, 0, 48);
  // lv_obj_set_style_bg_opa(r2, LV_OPA_TRANSP, 0);
  // lv_obj_set_style_border_width(r2, 3, 0);
  // lv_obj_set_style_border_color(r2, lv_color_white(), 0);
  // lv_obj_set_style_radius(r2, LV_RADIUS_CIRCLE, 0);
  // lv_obj_clear_flag(r2, LV_OBJ_FLAG_SCROLLABLE);
}

// Pressure Icon (Barometer style)
void create_pressure_icon(lv_obj_t *parent, lv_coord_t x, lv_coord_t y) {
  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_size(cont, 92, 110);
  lv_obj_align(cont, LV_ALIGN_CENTER, x, y);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

  // Outer dial
  lv_obj_t *dial = lv_obj_create(cont);
  lv_obj_set_size(dial, 78, 78);
  lv_obj_align(dial, LV_ALIGN_TOP_MID, 0, 8);
  lv_obj_set_style_bg_opa(dial, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(dial, 12, 0);
  lv_obj_set_style_border_color(dial, lv_color_hex(0x78909C), 0);
  lv_obj_set_style_radius(dial, LV_RADIUS_CIRCLE, 0);

  // Inner dial
  lv_obj_t *inner = lv_obj_create(cont);
  lv_obj_set_size(inner, 52, 52);
  lv_obj_align(inner, LV_ALIGN_TOP_MID, 0, 21);
  lv_obj_set_style_bg_color(inner, lv_color_hex(0x263238), 0);
  lv_obj_set_style_radius(inner, LV_RADIUS_CIRCLE, 0);

  // Needle
  lv_obj_t *needle = lv_obj_create(cont);
  lv_obj_set_size(needle, 6, 36);
  lv_obj_align(needle, LV_ALIGN_TOP_MID, 0, 23);
  lv_obj_set_style_bg_color(needle, lv_color_hex(0xFF7043), 0);
  lv_obj_set_style_transform_pivot_x(needle, 3, 0);
  lv_obj_set_style_transform_angle(needle, 280, 0);   // Pointing to a value

  // Center knob
  lv_obj_t *knob = lv_obj_create(cont);
  lv_obj_set_size(knob, 14, 14);
  lv_obj_align(knob, LV_ALIGN_TOP_MID, 0, 48);
  lv_obj_set_style_bg_color(knob, lv_color_white(), 0);
  lv_obj_set_style_radius(knob, LV_RADIUS_CIRCLE, 0);

}

