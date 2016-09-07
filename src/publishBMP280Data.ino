#include <Arduino.h>

//--------------------- Starting initialization code for Network
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#define WiFi_SSID "Rogers13486"
#define WiFi_PASS "CDE59Rogers"
#define Srvr_ConnectionString "http://192.168.0.19:3000"

ESP8266WiFiMulti WiFiMulti;

//--------------------- Starting initialization code for BMP280
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// Define the pins we will use for SPI connection to BMP280
#define BMP_SCK 5
#define BMP_MISO 12
#define BMP_MOSI 4
#define BMP_CS 13

Adafruit_BMP280 bme(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);

//-------------- Starting Setup Function
void setup() {

    Serial.begin(9600);

    // Try turning on the BMP280
    if (!bme.begin()) {
     Serial.println("Could not find a valid BMP280 sensor, check wiring!");
     while (1); // Crash on failure
    }

    Serial.println();
    Serial.println();
    Serial.println();

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }

    WiFiMulti.addAP(WiFi_SSID, WiFi_PASS);

}

// This function takes the result of a successful http request
// It processes the body of the response.
void _processSuccessfulSrvrResponse(String payload){
  Serial.print("Got Payload: ");
  Serial.println(payload);

  Serial.println("No further actions known for payload.");
}

int generateSensorString(char *output){
  float pressure = bme.readPressure();
  float temperature = bme.readTemperature();

  return sprintf(output,
    "sensor_feed/bmp280/1/%d.%02dkPa/%d.%02doC",
    (int)  pressure  , (int)( pressure  *100)%100,
    (int) temperature, (int)(temperature*100)%100
  );
}

void loop() {
    // wait for WiFi connection
    if((WiFiMulti.run() == WL_CONNECTED)) {
        HTTPClient http;

        Serial.print("[HTTP] begin...\n");

        char requestString[200];
        char sensorString[100];
        generateSensorString(sensorString);
        Serial.print(sensorString);

        sprintf(requestString,
          "%s/%s",
          Srvr_ConnectionString,
          sensorString
        );

        Serial.print("  Requesting address: ");
        Serial.println(requestString);
        http.begin(requestString); //HTTP

        Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                _processSuccessfulSrvrResponse(payload);
            }
        } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }

    delay(60000);
}
