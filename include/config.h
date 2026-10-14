#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <vector>

// --- Timezone ---
const char* const TZ_INFO = "GMT0BST,M3.5.0/1,M10.5.0/2";

// --- TfL stops ---
// Stop IDs, display labels and bus-route filters live in include/stops.h,
// which is gitignored. Copy include/stops.h.example to include/stops.h and fill it in.
#include "stops.h"

// --- Hardware pins (DFRobot FireBeetle 2 ESP32-E) ---
const int PIN_BAT_ADC  = A2; // GPIO34, confirmed ADC1 - safe to read with WiFi active
const int PIN_EPD_CS   = 13;
const int PIN_EPD_DC   = 22;
const int PIN_EPD_RST  = 21;
const int PIN_EPD_BUSY = 14;

// --- Battery calibration ---
// Onboard divider on the FireBeetle 2 ESP32-E is 1M+1M (i.e. exactly 2.0x),
// confirmed against the sibling Weather_Picture_Frame project which uses the
// same board. Percentage uses a sigmoidal approximation of the LiPo discharge
// curve (see battery_utils.cpp) rather than a linear map.
const float BATTERY_DIVIDER_RATIO   = 2.0f;
const uint32_t MIN_BATTERY_VOLTAGE_MV = 3000;
const uint32_t MAX_BATTERY_VOLTAGE_MV = 4200;
// Below this, the board shows a "battery critical" screen and stops
// scheduling further wakeups entirely (esp_deep_sleep_start() with no timer
// armed) rather than keep trying to fetch/refresh on a dying battery -
// deliberately not a graduated slowdown, see feedback memory on why.
const uint32_t CRIT_BATTERY_VOLTAGE_MV = 3350;

// --- Deep sleep schedule ---
const int NIGHT_START_HOUR    = 23; // 23:00-06:00
const int DAY_START_HOUR      = 6;
const int EVENING_START_HOUR  = 20; // 20:00-23:00
const int SLEEP_MIN_NIGHT     = 30;
const int SLEEP_MIN_EVENING   = 15;
const int SLEEP_MIN_DAY       = 5;

// --- Time sync ---
// The RTC clock keeps ticking (accurately enough) through deep sleep, so a
// fresh NTP sync every 5-minute wake is unnecessary - drift over a day is a
// few seconds at most, well under what matters for train times.
const time_t NTP_RESYNC_INTERVAL_SEC = 24 * 60 * 60;

// --- HTTP fetch ---
const unsigned long HTTP_TIMEOUT_MS = 30000;
const int HTTP_MAX_RETRIES = 3;
const unsigned long HTTP_RETRY_DELAY_MS = 500;

// --- Departure filtering / caching ---
// Minimum time-to-arrival for a departure to be shown. Kept small (rather than
// the original 2 minutes) so an imminent train isn't hidden for most of a
// 5-minute refresh cycle.
const long MIN_SECONDS_TO_ARRIVAL = 30;
// If a fetch fails, cached data from the previous successful fetch is shown
// instead of a blank/error section, as long as it's not older than this.
const time_t CACHE_MAX_AGE_SEC = 20 * 60;
// Fast partial e-ink refresh is used most cycles; a full refresh is forced at
// least this often to clear any accumulated ghosting.
const time_t FULL_REFRESH_INTERVAL_SEC = 60 * 60;

// --- Shared data types ---
struct Departure {
  String destination;
  String line;
  time_t arrivalRaw;
  int hour;
  int minute;
};

struct DepartureResult {
  std::vector<Departure> departures;
  String statusMessage;   // non-empty = show this message instead of the list (e.g. "No Data", "Data error")
  bool isStale = false;   // true if this is cached data from a previous cycle, not a fresh fetch
};

#endif
