// Including this library will also include lvgl.h and TFT_eSPI.h 
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

// LVGL objects
static lv_obj_t * time_label;
static lv_obj_t * date_label;
static lv_obj_t * temp_label;
static lv_obj_t * hum_label;
static lv_obj_t * press_label;

// ================== CONFIG ==================
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// Timezone offset in seconds (e.g. GMT+1 = 3600)
const long utcOffsetInSeconds = 3600;   

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 60000);

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;


// 1. Define your custom time-telling function
static uint32_t my_tick_get_cb(void) {
    return millis(); // Use Arduino's millis(), or HAL_GetTick() for STM32
}

void setup() {
  
  // Start Serial and lvgl, set everything up for this device
  // Will also do Serial.begin(115200), lvgl.init(), set up the touch driver
  // and the lvgl timer.
  LVGL_CYD::begin(USB_LEFT);
  LVGL_CYD::backlight(255);

  // lv_tick_set_cb(my_tick_get_cb);

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

  // Sensors
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

    Serial.println("Time");
    Serial.println(time_str);
    lv_label_set_text(time_label, time_str.c_str());

    String date_str = getDateString();
    Serial.println(date_str);
    lv_label_set_text(date_label, date_str.c_str());

    Serial.println("Humidity");
    // if (aht.getEvent(&humidity, &temp_aht)) {
    //     char temp_str[32];
    //     snprintf(temp_str, sizeof(temp_str), "Temp: %.1f °C", temp_aht.temperature);
    //     lv_label_set_text(temp_label, temp_str);

    //     char hum_str[32];
    //     snprintf(hum_str, sizeof(hum_str), "Hum: %.1f %%", humidity.relative_humidity);
    //     lv_label_set_text(hum_label, hum_str);
    // }

    // Serial.println("BMP");
    // lv_label_set_text(press_label, press_str);

    Serial.println("Exiting Update");


}    

String getDateString() {
  time_t rawTime = timeClient.getEpochTime();
  struct tm *timeinfo = localtime(&rawTime);
  
  char buffer[40];
  strftime(buffer, sizeof(buffer), "  %A, %B %d %Y  ", timeinfo);
  return String(buffer);
}

void loop() {

  // lvgl needs this to be called in a loop to run the interface
  lv_task_handler();
  delay(5);

}