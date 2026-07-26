#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

#include "config.h"
#include "battery_utils.h"
#include "client_utils.h"
#include "renderer.h"
#include "Icons/wifi_x_196x196.h"
#include "Icons/battery_alert_0deg_196x196.h"

namespace {

// Tracks when the e-paper panel last did a true full refresh, so most cycles
// can use the much faster/lower-power partial refresh instead (see
// FULL_REFRESH_INTERVAL_SEC in config.h) while still periodically clearing
// any ghosting. Survives deep sleep in RTC memory.
RTC_DATA_ATTR time_t g_lastFullRefresh = 0;

// Tracks the last successful NTP sync, so most cycles can skip the network
// round-trip and just trust the RTC clock for a bounded window (see
// NTP_RESYNC_INTERVAL_SEC in config.h). Survives deep sleep in RTC memory.
RTC_DATA_ATTR time_t g_lastNtpSync = 0;

int calcSleepMinutes(int hour) {
  if (hour >= NIGHT_START_HOUR || hour < DAY_START_HOUR) return SLEEP_MIN_NIGHT;
  if (hour >= EVENING_START_HOUR) return SLEEP_MIN_EVENING;
  return SLEEP_MIN_DAY;
}

void goToSleep(int sleepMinutes) {
  Serial.printf("Entering deep sleep for %d minutes...\n", sleepMinutes);
  esp_sleep_enable_timer_wakeup((uint64_t)sleepMinutes * 60 * 1000000ULL);
  esp_deep_sleep_start();
}

// Holds the onboard LED off through deep sleep. Without the gpio_hold calls,
// the pin can glitch high briefly during the sleep transition even after a
// plain digitalWrite(LOW). Belt-and-braces alongside the LED trace being cut
// on the board itself.
void disableBuiltinLED() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  gpio_hold_en(static_cast<gpio_num_t>(LED_BUILTIN));
  gpio_deep_sleep_hold_en();
}

} // namespace

void setup() {
  setCpuFrequencyMhz(80);
  Serial.begin(115200);
  disableBuiltinLED();

  // Read the battery before WiFi/display init draw any current, so the
  // reading reflects a quiet power rail and a resting (not load-sagged)
  // voltage.
  uint32_t batteryMv = readBatteryVoltageMv();
  int batteryPercent = calcBatteryPercent(batteryMv);
  Serial.printf("Battery: %d%% (%umV)\n", batteryPercent, batteryMv);

  bool wokeFromDeepSleep = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER);
  initDisplay(wokeFromDeepSleep);

  if (batteryMv <= CRIT_BATTERY_VOLTAGE_MV) {
    // Below this, don't even attempt WiFi - just show the message and stop
    // scheduling wakeups entirely, rather than keep draining a dying battery
    // trying to fetch data that's not particularly useful at this point.
    // Only a manual reset (after recharging) brings it back.
    Serial.println("Battery critical - showing error on display and hibernating.");
    drawStatusMessage("Battery Critical", "Recharge & reset", battery_alert_0deg_196x196);
    esp_deep_sleep_start();
    return;
  }

  wl_status_t wifiStatus = connectWiFi();
  if (wifiStatus != WL_CONNECTED) {
    if (wifiStatus == WL_NO_SSID_AVAIL) {
      Serial.println("WiFi network not found - showing error on display.");
      drawStatusMessage("Network Not Available", "", wifi_x_196x196);
    } else {
      Serial.println("Wi-Fi failed - showing error on display.");
      drawStatusMessage("WiFi Connection Failed", "", wifi_x_196x196);
    }
    goToSleep(5);
    return;
  }

  // TZ rules aren't persisted across deep sleep (unlike the RTC clock itself),
  // so this needs to run every boot - but it's a local libc call, no network.
  setenv("TZ", TZ_INFO, 1);
  tzset();

  // The internal RTC oscillator (this board has no external 32.768kHz
  // crystal) drifts by minutes over the course of a day - measured ~3m36s
  // over 10h17m, i.e. a few seconds per hour - so it needs resyncing far
  // more often than once a day (see NTP_RESYNC_INTERVAL_SEC in config.h).
  time_t nowBeforeSync = time(nullptr);
  bool needsNtpSync = (g_lastNtpSync == 0) ||
                       (difftime(nowBeforeSync, g_lastNtpSync) >= (double)NTP_RESYNC_INTERVAL_SEC);

  struct tm timeinfo;
  bool timeSynced = false;

  if (needsNtpSync) {
    configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");
    for (int i = 0; i < 10; i++) {
      if (getLocalTime(&timeinfo)) {
        timeSynced = true;
        break;
      }
      Serial.println("Waiting for NTP sync...");
      delay(500);
    }
    if (timeSynced) {
      g_lastNtpSync = time(nullptr);
      Serial.println("NTP synced.");
    }
  } else {
    timeSynced = getLocalTime(&timeinfo);
    Serial.println("Skipping NTP sync (synced within the last hour) - using RTC clock.");
  }

  if (!timeSynced) {
    Serial.println("Time sync failed - skipping update.");
    disconnectWiFi();
    drawStatusMessage("No time sync - retrying in 5m");
    goToSleep(5);
    return;
  }

  Serial.printf("Free heap before fetch: %d\n", ESP.getFreeHeap());

  DepartureResult results[5];
  results[0] = fetchDepartures(0, STOP_TRAIN, true, "");
  results[1] = fetchDepartures(1, STOP_BUS_1, false, LINES_BUS_1);
  results[2] = fetchDepartures(2, STOP_BUS_2, false, LINES_BUS_2);
  results[3] = fetchDepartures(3, STOP_BUS_3, false, LINES_BUS_3);
  results[4] = fetchDepartures(4, STOP_BUS_4, false, LINES_BUS_4);

  Serial.printf("Free heap after fetch: %d\n", ESP.getFreeHeap());

  // Nothing from here on needs the network - turn the radio off before the
  // display draw/refresh, which can take a few seconds on its own.
  disconnectWiFi();

  getLocalTime(&timeinfo); // fresh local time for display, now that fetching is done
  time_t now = time(nullptr);

  bool doFullRefresh = (g_lastFullRefresh == 0) ||
                        (difftime(now, g_lastFullRefresh) >= (double)FULL_REFRESH_INTERVAL_SEC);
  if (doFullRefresh) g_lastFullRefresh = now;

  drawBoard(results, batteryPercent, timeinfo.tm_hour, timeinfo.tm_min, /*useFastPartialUpdate=*/!doFullRefresh);

  goToSleep(calcSleepMinutes(timeinfo.tm_hour));
}

void loop() {
  // setup() always ends in deep sleep, which restarts the ESP32 - loop() is never reached.
}
