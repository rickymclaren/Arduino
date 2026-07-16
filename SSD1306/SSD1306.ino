#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Reset pin (-1 = no reset pin)
#define OLED_RESET    -1

// Custom I2C pins
#define SDA_PIN 27
#define SCL_PIN 22

// Create display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize the OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(true);

  // Welcome message
  display.setTextSize(2);
  display.setCursor(15, 8);
  display.println("ESP32");

  display.setTextSize(1);
  display.setCursor(5, 35);
  display.println("SDA:27  SCL:22");

  display.setCursor(10, 50);
  display.println("OLED Ready!");

  display.display();

  Serial.println("OLED initialized on custom pins (SDA=27, SCL=22)");
}

void loop() {
  static int counter = 0;

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 + SSD1306");

  display.setTextSize(2);
  display.setCursor(45, 20);
  display.print(counter);

  // Progress bar
  display.drawRect(0, 52, 128, 12, SSD1306_WHITE);
  display.fillRect(2, 54, (counter % 124), 8, SSD1306_WHITE);

  display.display();

  counter++;
  delay(180);
}