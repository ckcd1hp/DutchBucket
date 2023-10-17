#include <AsyncElegantOTA.h> // must be declared once as library instantiates class in .h
#include "wifiUtil.h"
#include "config.h"

// create AsyncWebServer on port 80
AsyncWebServer server(80);
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET_IN_SECONDS, NTP_UPDATE_INTERVAL);
ESP32Time rtc;           // no offset, as that is already added from NTPClient
String lastNTPSync = ""; // update time from NTP server and sync to RTC

unsigned long wifiPrevMillis = 0;

void setupWifi()
{
    // delete old config
    WiFi.disconnect(true);
    delay(1000);
    // add wifi events
    WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    // WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.mode(WIFI_STA); // station mode: ESP32 connects to access point
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Connecting to WIFI");
    delay(10000);
    timeClient.begin();

    AsyncElegantOTA.begin(&server);
    server.begin();
}
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    // mdns responder for esp32.local
    if (MDNS.begin("MDNS_NAME"))
    {
        Serial.println("MDNS responder started, accessible via nftv2.local");
    }
    delay(2000);
    // The function timeClient.update() syncs the local time to the NTP server. In the video I call this in the main loop. However, NTP servers dont like it if
    // they get pinged all the time, so I recommend to only re-sync to the NTP server occasionally. In this example code we only call this function once in the
    // setup() and you will see that in the loop the local time is automatically updated. Of course the ESP/Arduino does not have an infinitely accurate clock,
    // so if the exact time is very important you will need to re-sync once in a while.
    updateAndSyncTime(); // anytime esp32 reconnects to wifi it will attempt to sync time
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("Disconnected from WiFi access point");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    WiFi.disconnect(true);
}
void checkWifiStatus()
{
    unsigned long currentMillis = millis();
    // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
    if ((WiFi.status() != WL_CONNECTED) && (currentMillis - wifiPrevMillis >= WIFI_RETRY_WAIT_TIME))
    {
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
        wifiPrevMillis += WIFI_RETRY_WAIT_TIME;
    }
}
void updateAndSyncTime()
{
    if (timeClient.update())
    {
        // successful update
        Serial.println("Recieved updated time from NTP!");
        // set RTC time
        rtc.setTime(timeClient.getEpochTime());
        lastNTPSync = rtc.getTime("%A, %B %d %Y %I:%M %p");
        Serial.println("Updated RTC: " + lastNTPSync);
    }
    else
    {
        // unsuccessful update, display current unsynced RTC time
        Serial.println("Unable to connect to NTP or already updated within the last 30 minutes");
        Serial.println("RTC: " + rtc.getTime("%A, %B %d %Y %I:%M %p"));
    }
}