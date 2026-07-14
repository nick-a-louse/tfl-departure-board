#include "client_utils.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <algorithm>

#include "secrets.h"
#include "time_utils.h"

namespace {

bool compareDepartures(const Departure& a, const Departure& b) {
  return a.arrivalRaw < b.arrivalRaw;
}

bool lineMatches(const String& filterLines, const String& lineName) {
  if (filterLines.length() == 0) return true;
  int start = 0;
  int comma = filterLines.indexOf(',', start);
  while (comma != -1) {
    if (filterLines.substring(start, comma) == lineName) return true;
    start = comma + 1;
    comma = filterLines.indexOf(',', start);
  }
  return filterLines.substring(start) == lineName;
}

// --- Last-known-good cache (RTC memory: survives deep sleep, cleared on a
//     full power loss/reset - which is fine, that's a rare event compared to
//     routine wake cycles, and avoids wearing the flash-backed NVS by writing
//     it every few minutes). ---
const int MAX_CACHED_PER_STOP = 5;
const int NUM_STOPS = 5;

struct CachedDeparture {
  char destination[32];
  char line[8];
  uint8_t hour;
  uint8_t minute;
};

struct StopCache {
  CachedDeparture entries[MAX_CACHED_PER_STOP];
  uint8_t count;
  time_t cachedAt;
  bool valid;
};

RTC_DATA_ATTR StopCache g_stopCache[NUM_STOPS];

void updateCache(int stopIndex, const std::vector<Departure>& departures) {
  if (stopIndex < 0 || stopIndex >= NUM_STOPS) return;
  StopCache& cache = g_stopCache[stopIndex];
  cache.count = (uint8_t)std::min((size_t)MAX_CACHED_PER_STOP, departures.size());
  for (uint8_t i = 0; i < cache.count; i++) {
    strncpy(cache.entries[i].destination, departures[i].destination.c_str(), sizeof(cache.entries[i].destination) - 1);
    cache.entries[i].destination[sizeof(cache.entries[i].destination) - 1] = '\0';
    strncpy(cache.entries[i].line, departures[i].line.c_str(), sizeof(cache.entries[i].line) - 1);
    cache.entries[i].line[sizeof(cache.entries[i].line) - 1] = '\0';
    cache.entries[i].hour = (uint8_t)departures[i].hour;
    cache.entries[i].minute = (uint8_t)departures[i].minute;
  }
  cache.cachedAt = time(nullptr);
  cache.valid = true;
}

bool readCache(int stopIndex, std::vector<Departure>& outDepartures) {
  if (stopIndex < 0 || stopIndex >= NUM_STOPS) return false;
  StopCache& cache = g_stopCache[stopIndex];
  if (!cache.valid) return false;
  if (difftime(time(nullptr), cache.cachedAt) > (double)CACHE_MAX_AGE_SEC) return false;

  outDepartures.clear();
  for (uint8_t i = 0; i < cache.count; i++) {
    Departure d;
    d.destination = cache.entries[i].destination;
    d.line = cache.entries[i].line;
    d.hour = cache.entries[i].hour;
    d.minute = cache.entries[i].minute;
    d.arrivalRaw = 0; // not needed to redisplay a cached entry
    outDepartures.push_back(d);
  }
  return true;
}

// --- Shared client objects, reused across stops for convenience only - each
//     request still gets its own independent connection (no keep-alive).
//     Connection reuse was tried and pulled: a truncated ("IncompleteInput")
//     body was observed on real hardware, and correctness matters more here
//     than the handshake savings. ---
WiFiClientSecure g_httpsClient;
HTTPClient g_http;
bool g_httpReady = false;

void ensureHttpReady() {
  if (g_httpReady) return;
  g_httpsClient.setInsecure(); // matches original behaviour: no cert pinning
  g_httpReady = true;
}

// Fetches the raw response and parses it into `departures`. Returns "" on
// success, or a short user-facing reason ("No Data" / "Data error") on
// failure. Retries on both HTTP-level failure and a JSON parse failure -
// a truncated/malformed body on one attempt doesn't mean the next will be.
String fetchAndParse(const String& stopId, bool isTrain, const String& filterLines,
                      std::vector<Departure>& departures) {
  ensureHttpReady();
  String url = "https://api.tfl.gov.uk/StopPoint/" + stopId + "/Arrivals?app_key=" + String(tfl_app_key);
  String lastError = "No Data";

  for (int attempt = 0; attempt < HTTP_MAX_RETRIES; attempt++) {
    g_http.setTimeout(HTTP_TIMEOUT_MS);
    g_http.begin(g_httpsClient, url);
    int httpCode = g_http.GET();
    Serial.printf("HTTP %s: %d\n", stopId.c_str(), httpCode);

    if (httpCode == 200) {
      String payload = g_http.getString();
      g_http.end();

      JsonDocument filter;
      filter[0]["lineName"] = true;
      filter[0]["destinationName"] = true;
      filter[0]["expectedArrival"] = true;

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

      if (error) {
        Serial.printf("JSON parse failed for %s (attempt %d, %u bytes): %s\n",
                       stopId.c_str(), attempt + 1, payload.length(), error.c_str());
        lastError = "Data error";
        delay(HTTP_RETRY_DELAY_MS);
        continue;
      }
      Serial.printf("Parsed %u raw entries for %s\n", doc.as<JsonArray>().size(), stopId.c_str());

      time_t now = time(nullptr);
      departures.clear();

      for (JsonObject obj : doc.as<JsonArray>()) {
        String lineName = String((const char*)(obj["lineName"] | (isTrain ? "ELIZ" : "BUS")));
        if (!isTrain && filterLines.length() > 0 && !lineMatches(filterLines, lineName)) continue;

        const char* arrivalCStr = obj["expectedArrival"] | "";
        if (strlen(arrivalCStr) == 0) continue;

        time_t arrivalTime = parseISO8601UTC(arrivalCStr);
        if (arrivalTime == 0) continue;
        if (difftime(arrivalTime, now) < MIN_SECONDS_TO_ARRIVAL) continue;

        struct tm* lt = localtime(&arrivalTime);
        if (lt == nullptr) continue;

        String dest = String((const char*)(obj["destinationName"] | "Unknown"));
        dest.replace(" Rail Station", "");
        dest.replace(" (TfL)", "");
        dest.trim();

        departures.push_back({dest, lineName, arrivalTime, lt->tm_hour, lt->tm_min});
      }

      std::sort(departures.begin(), departures.end(), compareDepartures);
      return "";
    }

    g_http.end();
    delay(HTTP_RETRY_DELAY_MS);
  }

  return lastError;
}

} // namespace

wl_status_t connectWiFi() {
  Preferences prefs;
  prefs.begin("wifi", true);
  int savedChannel = prefs.getInt("channel", 0);
  uint8_t savedBssid[6];
  bool hasSaved = prefs.getBytes("bssid", savedBssid, 6) == 6;
  prefs.end();

  if (hasSaved && savedChannel > 0) {
    WiFi.begin(ssid, password, savedChannel, savedBssid);
  } else {
    WiFi.begin(ssid, password);
  }

  int attempt = 0;
  wl_status_t status = WiFi.status();
  while (status != WL_CONNECTED && attempt < 20) {
    delay(500);
    attempt++;
    status = WiFi.status();
  }

  if (status != WL_CONNECTED) return status;

  prefs.begin("wifi", false);
  prefs.putInt("channel", WiFi.channel());
  prefs.putBytes("bssid", WiFi.BSSID(), 6);
  prefs.end();

  return WL_CONNECTED;
}

void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
}

DepartureResult fetchDepartures(int stopIndex, const String& stopId, bool isTrain, const String& filterLines) {
  std::vector<Departure> departures;
  String failReason = fetchAndParse(stopId, isTrain, filterLines, departures);

  if (failReason.length() == 0) {
    updateCache(stopIndex, departures);
    DepartureResult result;
    result.departures = departures;
    return result;
  }

  std::vector<Departure> cached;
  if (readCache(stopIndex, cached)) {
    Serial.printf("Using cached data for stop index %d (%s)\n", stopIndex, stopId.c_str());
    DepartureResult result;
    result.departures = cached;
    result.isStale = true;
    return result;
  }

  DepartureResult result;
  result.statusMessage = failReason;
  return result;
}
