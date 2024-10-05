#include <Wire.h>

#include <WiFi.h>
#include "WiFiClientSecure.h"
WiFiClientSecure client;
#include "cert.h"

//#define i2c_Address 0x3d
//#define i2c_Address 0x3c    // cut jumper I2C Address on lthe left and solder on the right 
#define SEALEVELPRESSURE_HPA (1013.25)
#define DELAYTIME 1000
#define BME280_ADDRESS (0x77)   // (0x76) cut left and solder left pad on board

#include <Adafruit_Sensor.h>    // https://github.com/adafruit/Adafruit_Sensor
#include <Adafruit_BME280.h>    // https://github.com/adafruit/Adafruit_BME280_Library


Adafruit_BME280 bme; // I2C

#define MQTT_SERVER "k83ff1f3.ala.eu-central-1.emqxsl.com"
#define MQTT_PORT 8883
#define MQTT_USERNAME "test"
#define MQTT_PW "test"

#include <PubSubClient.h>
PubSubClient mqtt( MQTT_SERVER,MQTT_PORT, client);


void setup()   {
  //Wire.begin(8, 10); // 8,10 = ESP32-C3-LPKit v2
  Serial.begin(115200);
  delay(100); // let serial console settle
  Serial.println("WiFi Connect");
  //wifiManager.autoConnect("ssid", "moje_tajne_heslo");
  WiFi.begin("ssid", "moje_tajne_heslo");
  delay(2000);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
   Serial.println("WiFi connected");
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());
  client.setCACert(root_ca);


  delay(250); // wait for the OLED to power up

  Serial.println("Starting...");

    Wire.begin (8, 10); // for ESP32-C3-Lp + uŠup - SDA=8, SCL=10
    if (! bme.begin(BME280_ADDRESS)) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) delay(10);
    }

    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF   );

  delay(1000);
}


void loop() {
    MQTT_connect();
    String payload = "{\"temperature\":";
    payload.concat(String(bme.readTemperature()));
    payload.concat(",\"humidity\":");
    payload.concat(String(bme.readHumidity()));
    payload.concat(",\"pressure\":");
    payload.concat(String(bme.readPressure()));
    payload.concat("}");
    mqtt.publish("test/mine", payload.c_str());
  Serial.println(payload.c_str());
  delay(2000);
}


// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print(F("Connecting to MQTT... "));

  uint8_t retries = 3;
  while (!mqtt.connect(MQTT_USERNAME,MQTT_USERNAME,MQTT_PW)) { // connect will return 0 for connected
       //Serial.println(mqtt.connectErrorString(ret));
       Serial.println(F("Retrying MQTT connection in 5 seconds..."));
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println(F("MQTT Connected!"));
}