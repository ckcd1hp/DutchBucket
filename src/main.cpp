#include <Arduino.h>

#include "pins.h" // for pin declarations
#include "util.h" // for utility functions
#include "wifiUtil.h"

extern ESP32Time rtc; // access esp32 internal real time clock

int previousHour = -1; // track the last hour to run functions once an hour

void setup()
{
  Serial.begin(115200);
  // print reason for rebooting (debugging)
  // Serial.println("System Reset: " + printBootReason());
  Serial.println("Setup begin");

  setupWifi(); // setup wifi to nftv2.local, sync ntp, enable OTA
  // set pinout
  pinMode(LED_PIN, OUTPUT);
}

void loop()
{
  // checkWifiStatus();
  // get current hour 0-23
  int currentHour = rtc.getHour(true);
  /* --------------- HOUR CHANGE -------------------*/
  if (currentHour != previousHour)
  {
    // Update time using NTP at same time everyday
    if (currentHour == NTP_SYNC_HOUR)
      updateAndSyncTime();
    previousHour = currentHour;
  }