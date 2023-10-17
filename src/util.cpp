#include "util.h"

String printBootReason()
{
    esp_reset_reason_t reset_reason = esp_reset_reason();

    switch (reset_reason)
    {
    case ESP_RST_UNKNOWN:
        return "Reset reason can not be determined";
        break;
    case ESP_RST_POWERON:
        return "Reset due to power-on event";
        break;
    case ESP_RST_EXT:
        return "Reset by external pin (not applicable for ESP32)";
        break;
    case ESP_RST_SW:
        return "Software reset via esp_restart";
        break;
    case ESP_RST_PANIC:
        return "Software reset due to exception/panic";
        break;
    case ESP_RST_INT_WDT:
        return "Reset (software or hardware) due to interrupt watchdog";
        break;
    case ESP_RST_TASK_WDT:
        return "Reset due to task watchdog";
        break;
    case ESP_RST_WDT:
        return "Reset due to other watchdogs";
        break;
    case ESP_RST_DEEPSLEEP:
        return "Reset after exiting deep sleep mode";
        break;
    case ESP_RST_BROWNOUT:
        return "Brownout reset (software or hardware)";
        break;
    case ESP_RST_SDIO:
        return "Reset over SDIO";
        break;
    default:
        break; // test deep sleep cases
    }

    if (reset_reason == ESP_RST_DEEPSLEEP)
    {
        esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

        switch (wakeup_reason)
        {
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            return "In case of deep sleep: reset was not caused by exit from deep sleep";
            break;
        case ESP_SLEEP_WAKEUP_ALL:
            return "Not a wakeup cause: used to disable all wakeup sources with esp_sleep_disable_wakeup_source";
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            return "Wakeup caused by external signal using RTC_IO";
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            return "Wakeup caused by external signal using RTC_CNTL";
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            return "Wakeup caused by timer";
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            return "Wakeup caused by touchpad";
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            return "Wakeup caused by ULP program";
            break;
        case ESP_SLEEP_WAKEUP_GPIO:
            return "Wakeup caused by GPIO (light sleep only)";
            break;
        case ESP_SLEEP_WAKEUP_UART:
            return "Wakeup caused by UART (light sleep only)";
            break;
        default:
            break;
        }
    }
    return "Could not determine reset reason";
}