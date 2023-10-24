#include <Arduino.h>

#include "pins.h" // for pin declarations
#include "util.h" // for utility functions
#include "wifiUtil.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
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
// Telegram bot
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

Preferences preferences;

int floatSwitchState = 0;
// maximum temp of the reservoir recorded
float maxResTemp;
bool waterPumpCommand = false;

void setup()
{
  Serial.begin(115200);
  // print reason for rebooting (debugging)
  // Serial.println("System Reset: " + printBootReason());
  Serial.println("Setup begin");

  setupWifi(); // setup wifi to [mdns_name].local, sync ntp, enable OTA
  // set pinout
  pinMode(LED_PIN, OUTPUT);
  pinMode(FLOAT_SWITCH_PIN, INPUT_PULLUP);
  pinMode(WATER_PUMP_1_PIN, OUTPUT);
  sensors.begin();
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org

  // load saved data
  preferences.begin("dutchBucket", false);
  maxResTemp = preferences.getFloat("maxResTemp", 0);
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
    // get current hour 0-23
    int currentHour = rtc.getHour(true);
    // check to run pump for 1 min 3 times a day
    controlWaterPump(currentMinute, currentHour);
    // read reservoir temp every 15 minutes
    if (currentMinute % 15 == 0)
    {
      sensors.requestTemperatures();
      float tempF = sensors.getTempFByIndex(0);
      Serial.print(tempF);
      Serial.println("ºF");
      if (tempF > maxResTemp)
      {
        // load saved data
        preferences.begin("dutchBucket", false);
        maxResTemp = tempF;
        preferences.putFloat("maxResTemp", maxResTemp);
        preferences.end();
      }
    }
    /* --------------- HOUR CHANGE -------------------*/
    if (currentHour != previousHour)
    {
      // check float switch
      // Update time using NTP at same time everyday
      if (currentHour == NTP_SYNC_HOUR)
        updateAndSyncTime();
      previousHour = currentHour;
    }
    previousMinute = currentMinute;
  }
}
// run pump 3 times a day 1 minute at a time
void controlWaterPump(int currentHour, int currentMin)
{
  if (currentMin == 0 and (currentHour == 6 or currentHour == 12 or currentHour == 18))
  {
    waterPumpCommand = true;
  }
  else
  {
    waterPumpCommand = false;
  }
  // set pump1 command
  digitalWrite(WATER_PUMP_1_PIN, waterPumpCommand ? HIGH : LOW);
}
// check float switch once an hour during the day
void checkFloatSwitch(int currentHour)
{
  if (currentHour >= 6 and currentHour <= 18)
  {
    floatSwitchState = digitalRead(FLOAT_SWITCH_PIN);
    Serial.println(floatSwitchState);
    // alert user if float switch is tripped
    if (floatSwitchState)
    {
      // message user once at time of event and once at end of day (6pm)
        }
  }
}