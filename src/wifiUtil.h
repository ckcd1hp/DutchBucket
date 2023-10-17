#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESP32Time.h>
#include <NTPClient.h>

#define UTC_OFFSET_IN_SECONDS -36000 // offset from greenwich time (Hawaii is UTC-10)
#define NTP_SYNC_HOUR 4
#define WIFI_RETRY_WAIT_TIME 300000 // 5 minutes in milliseconds
#define NTP_UPDATE_INTERVAL 1800000 // 30 min in milliseconds (minimum retry time, normally daily)

void setupWifi();                                                      // run in setup to start Wifi, sync NTP, and start OTA web server
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);    // on wifi connect
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);               // on getting IP
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info); // on disconnect
void checkWifiStatus();                                                // periodically check if wifi has disconnected and try to reconnect
void updateAndSyncTime();                                              // update RTC with NTP server