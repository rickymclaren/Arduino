#include <SPI.h>
#include <SD.h>

File file;

void setup() {
  Serial.begin(9600);
  Serial.println("Initialising SD card...");

  if (!SD.begin(10)) {
    Serial.println("Initialising SD card failed");
    return;
  }

  Serial.println("Initialising SD card successful");

  file=SD.open("hello.txt");
  if (file) {
    while (file.available()) {
      Serial.write(file.read());
    }
  }
  file.close();

  file=SD.open("reply.txt", FILE_WRITE);
  file.println("Hello from Nano again");
  file.close();

  Serial.println("reply.txt created");
}

void loop() {
  // put your main code here, to run repeatedly:

}
