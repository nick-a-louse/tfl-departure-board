#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>

// Parses a TfL "expectedArrival"-style ISO8601 UTC timestamp
// ("YYYY-MM-DDTHH:MM:SS...") into a UTC epoch time_t. Returns 0 on failure.
time_t parseISO8601UTC(const char* isoTime);

#endif
