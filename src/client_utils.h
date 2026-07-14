#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

// Connects to WiFi, using a cached channel/BSSID (via Preferences) to skip a
// full AP scan when possible. Returns WL_CONNECTED on success, or the
// wl_status_t reason for failure (notably WL_NO_SSID_AVAIL when the
// configured network isn't visible, as distinct from any other failure).
wl_status_t connectWiFi();

// Disconnects and powers off the WiFi radio.
void disconnectWiFi();

// Fetches and parses departures for one stop. On failure (after retries),
// falls back to the last successful result for this stop if it's still
// within CACHE_MAX_AGE_SEC, otherwise returns a result with statusMessage set.
// stopIndex identifies which of the 5 boxes this is (0-4), for the
// RTC-memory last-known-good cache.
DepartureResult fetchDepartures(int stopIndex, const String& stopId, bool isTrain, const String& filterLines);

#endif
