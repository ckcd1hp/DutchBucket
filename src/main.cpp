#include <Arduino.h>

#include "pins.h" // for pin declarations
#include "util.h" // for utility functions
#include "bot.h"  // for bot related functions
#include "wifiUtil.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// function declarations
void handleNewMessages(int numNewMessages);             // handling new messages from telegram user
void controlWaterPump(int currentHour, int currentMin); // turn on water pump 3 times a day for a minute each
void checkFloatSwitch(int currentHour);                 // check float switch twice a day and warn user if water level is low

#define WATER_PUMP_OVERRIDE_TIME 60000 // 1 minute override time

extern ESP32Time rtc; // access esp32 internal real time clock

int previousHour = -1;   // track the last hour to run functions once an hour
int previousMinute = -1; // track the last minute to run functions once on the new minute

// oneWire instance to communicate with any OneWire devices
OneWire oneWire(DS18B20_TEMP_PIN);
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
bool overridePump = false;
unsigned long pumpOverrideMillis = 0;

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
  int startCounter = preferences.getUInt("startCounter", 0);
  if (startCounter != 0)
  {
    String message = "Dutch Bucket - Reset counter: " + String(startCounter) + ", " + printBootReason();
    bot.sendMessage(CHAT_ID, message); // send restart bot message
  }
  else
    bot.sendMessage(CHAT_ID, BOT_GREETING_MESSAGE); // send bot greeting message
  // update esp32 start counter
  startCounter++;
  preferences.putUInt("startCounter", startCounter);
  Serial.println("Max Res Temp: " + String(maxResTemp));
  preferences.end();
}

void loop()
{
  if (overridePump)
    runPumpInManual();

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
// run pump in override for 1 minute
void runPumpInManual()
{
  unsigned long now = millis(); // get current millis
  if (now - pumpOverrideMillis > WATER_PUMP_OVERRIDE_TIME)
  {
    // turn pump off
    overridePump = false;
    // set pump1 command
    digitalWrite(WATER_PUMP_1_PIN, LOW);
  }
}
// check float switch at the beginning and end of day
void checkFloatSwitch(int currentHour)
{
  if (currentHour == 6 or currentHour == 18)
  {
    floatSwitchState = digitalRead(FLOAT_SWITCH_PIN);
    Serial.println(floatSwitchState);
    // alert user if float switch is tripped
    if (floatSwitchState)
    {
      // message user
      bot.sendMessage(CHAT_ID, BOT_LOW_WATER_MESSAGE); // send bot greeting message
    }
  }
}
// Handle what happens when you receive new messages from telegram bot
void handleNewMessages(int numNewMessages)
{
  // WebSerial.println("New messages from telegram: " + String(numNewMessages));
  for (int i = 0; i < numNewMessages; i++)
  {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID)
    {
      bot.sendMessage(chat_id, BOT_UNAUTHORIZED_MESSAGE);
      continue;
    }
    // Print the received message
    String msg = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    if (msg == "/help")
    {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your outputs.\n\n";
      welcome += "/led_on to turn GPIO ON \n";
      welcome += "/led_off to turn GPIO OFF \n";
      welcome += "/state to request current GPIO state \n";
      bot.sendMessage(chat_id, welcome, "");
    }
    else if (msg == "/dutchrun")
    {
      overridePump = true;
      pumpOverrideMillis = millis();
      // set pump1 command
      digitalWrite(WATER_PUMP_1_PIN, HIGH);
      bot.sendMessage(chat_id, "Running water pump for 1 minute");
    }
    else if (msg == "/dutchtemp")
    {
      sensors.requestTemperatures();
      float tempF = sensors.getTempFByIndex(0);
      String message = "The current water temp is " + String(tempF) + "ºF";
      bot.sendMessage(chat_id, message);
    }
    else if (msg == "/resetnutrients")
    {
      // load saved data
      // preferences.begin("nft", false);
      // nutrientReminderEpoch = rtc.getEpoch();
      // nutrientReminderEpoch = preferences.putULong64("nRE", nutrientReminderEpoch);
      // preferences.end();
    }
    else
    {
      bot.sendMessage(chat_id, "That is not a valid command.");
    }
  }
}