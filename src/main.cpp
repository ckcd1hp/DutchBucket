#include <Arduino.h>

#include "pins.h" // for pin declarations
#include "util.h" // for utility functions
#include "wifiUtil.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>

extern ESP32Time rtc; // access esp32 internal real time clock

int previousHour = -1;   // track the last hour to run functions once an hour
int previousMinute = -1; // track the last minute to run functions once on the new minute

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;
// oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass oneWire reference to Dallas Temp Sensor
DallasTemperature sensors(&oneWire);

Preferences preferences;

int floatSwitchState = 0;

void setup()
{
  Serial.begin(115200);
  // print reason for rebooting (debugging)
  // Serial.println("System Reset: " + printBootReason());
  Serial.println("Setup begin");

  setupWifi(); // setup wifi to [mdns_name].local, sync ntp, enable OTA
  // set pinout
  pinMode(LED_PIN, OUTPUT);
  pinMode(FLOAT_SWITCH, INPUT_PULLUP);
  sensors.begin();

  // load saved data
  preferences.begin("dutchBucket", false);
  float maxResTemp = preferences.getFloat("maxResTemp", 0);
  Serial.println("Max Res Temp: " + String(maxResTemp));
  preferences.end();
}

void loop()
{
  // checkWifiStatus();
  int currentMinute = rtc.getMinute();
  /* --------------- MINUTE CHANGE -------------------*/
  if (currentMinute != previousMinute)
  {
    // read reservoir temp every 15 minutes
    if (currentMinute % 15 == 0)
    {
      sensors.requestTemperatures();
      float tempF = sensors.getTempFByIndex(0);
      Serial.print(tempF);
      Serial.println("ºF");
      // load saved data
      preferences.begin("dutchBucket", false);
      float maxResTemp = preferences.getFloat("maxResTemp", 0);
      if (tempF > maxResTemp)
      {
        maxResTemp = tempF;
        preferences.putFloat("maxResTemp", maxResTemp);
      }
      preferences.end();
    }
    // get current hour 0-23
    int currentHour = rtc.getHour(true);
    /* --------------- HOUR CHANGE -------------------*/
    if (currentHour != previousHour)
    {
      // check float switch once an hour
      floatSwitchState = digitalRead(FLOAT_SWITCH);
      Serial.println(floatSwitchState);
      // Update time using NTP at same time everyday
      if (currentHour == NTP_SYNC_HOUR)
        updateAndSyncTime();
      previousHour = currentHour;
    }
    previousMinute = currentMinute;
  }
}