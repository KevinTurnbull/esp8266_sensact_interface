#include <Arduino.h>

//--------------------- Starting initialization code for Network
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#define WiFi_SSID "Rogers13486"
#define WiFi_PASS "CDE59Rogers"
#define Srvr_ConnectionString "http://192.168.0.10:3000"

ESP8266WiFiMulti WiFiMulti;

//--------------------- Starting initialization code for DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>

// Define the pins we will use for OneWire connection to DS18B20
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 12

// Define the pin we will use for controlling the SousVide
#define LED_PIN 13
// Set up the OneWire connection to the DS18B20
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to the Dallas Temperature library.
DallasTemperature sensors(&oneWire);
// arrays to hold device address
DeviceAddress insideThermometer;

//-------------- Starting Setup Function
void setup() {

    Serial.begin(9600);

    Serial.println("--------RESETTING");
    Serial.println();
    Serial.println();

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }

    WiFiMulti.addAP(WiFi_SSID, WiFi_PASS);

    // Set up the LED_PIN as an OUTPUT
    pinMode(LED_PIN, OUTPUT);
    // Insuring it starts low.
    digitalWrite(LED_PIN, LOW);Serial.print("Confirming - Setting Low\n");

    // Start Setting up the DS18B20
    Serial.println("Dallas Temperature IC Control Library Demo");

    // locate devices on the bus
    Serial.print("Locating devices...");
    sensors.begin();
    Serial.print("Found ");
    Serial.print(sensors.getDeviceCount(), DEC);
    Serial.println(" devices.");

    if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");

    // show the addresses we found on the bus
    Serial.print("Device 0 Address: ");
    printAddress(insideThermometer);
    Serial.println();

    // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
    sensors.setResolution(insideThermometer, 9);

    Serial.print("Device 0 Resolution: ");
    Serial.print(sensors.getResolution(insideThermometer), DEC);
    Serial.println();
}

struct SousVideAction {
  int heatOnTimer;
  int heatOffTimer;
};

// This function takes the result of a successful http request
// It processes the body of the response and returns a SousVideAction
struct SousVideAction _processSuccessfulSrvrResponse(String payload){
  struct SousVideAction result;
  result.heatOnTimer = 0;
  result.heatOffTimer = 0;

  Serial.print("Got Payload: ");
  Serial.println(payload);

  if (payload.charAt(0) == 'H') { // The command is a Hard Set of the heater
    if (payload.charAt(1) == '1') { // to ON
      result.heatOnTimer=10000;
    } else {  // to OFF
      result.heatOffTimer=10000;
    }
  } else if (payload.charAt(0) == 'T') { // The command is a timer set of the heater
    result.heatOnTimer =  payload.substring(1,2).toInt() * 1000;
    result.heatOffTimer = payload.substring(2,3).toInt() * 1000;
  }

  return result;
}

int generateSensorString(char *output){
  float tempC = sensors.getTempC(insideThermometer);

  return sprintf(output,
    "sous_vide/ds18b20/%d.%doC",
    (int) tempC, (int)(tempC*100)%100
  );
}

void loop() {
    // wait for WiFi connection
    if(WiFiMulti.run() != WL_CONNECTED) {
        Serial.print("WiFiMulti.run != WL_CONNECTED\n");
    } else {
        HTTPClient http;

        for (int i = 1 ; i <= 1 ; i++)
        {
          Serial.print("[HTTP] begin...\n");

          char requestString[200];
          char sensorString[100];

          sensors.requestTemperatures(); // Send the command to get temperatures
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
                  struct SousVideAction action = _processSuccessfulSrvrResponse(payload);

                  if (action.heatOnTimer > 0)
                  {
                    digitalWrite(LED_PIN, HIGH);Serial.print("Confirming - Writing High on Timer\n");
                    delay(action.heatOnTimer);
                  }
                  if (action.heatOffTimer > 0)
                  {
                    digitalWrite(LED_PIN, LOW);Serial.print("Confirming - Setting Low on Timer\n");
                    delay(action.heatOffTimer);
                  }
              }
          } else {
              Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
          }

          http.end();
        }
    }
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
