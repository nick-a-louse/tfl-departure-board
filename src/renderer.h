#ifndef RENDERER_H
#define RENDERER_H

#include <Arduino.h>
#include "config.h"

// Initializes the e-paper display hardware. Call once per boot.
// isWakeFromDeepSleep must be true only when waking from deep sleep with the
// panel's power kept on throughout - GxEPD2 needs this to avoid forcing a
// full LUT re-init that silently overrides fast partial update on every
// cycle (see initDisplay's definition for the full explanation).
void initDisplay(bool isWakeFromDeepSleep);

// Draws the full board. `results` must have exactly 5 entries, in order:
// train, busPR, busPA, busAP, busAC. `useFastPartialUpdate` selects the
// e-paper refresh mode (fast/partial vs full flashing refresh) - the drawn
// content is identical either way, only the panel's physical refresh differs.
void drawBoard(const DepartureResult results[5], int batteryPercent, int hour24, int minute,
                bool useFastPartialUpdate);

// Draws a full-screen status message (WiFi/NTP failure, critical battery).
// icon196, if non-null, must point to a 196x196 1bpp bitmap (PROGMEM) and is
// drawn above large centered text; with icon196 null, falls back to small
// top-left text (used for the plain NTP-failure message).
void drawStatusMessage(const String& line1, const String& line2 = "", const uint8_t* icon196 = nullptr);

#endif
