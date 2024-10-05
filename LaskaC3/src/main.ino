#include <Wire.h>

#include <WiFi.h>

#define i2c_Address 0x3d
//#define i2c_Address 0x3c    // cut jumper I2C Address on lthe left and solder on the right 


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

const char* ssid = "ssid";
const char* password = "moje_tajne_heslo";

void setup()   {
  Wire.begin(8, 10); // 8,10 = ESP32-C3-LPKit v2
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  delay(250); // wait for the OLED to power up

  Serial.println("Starting...");

  delay(1000);
}


void loop() {
  Serial.println(WiFi.RSSI());
  delay(1000);
}