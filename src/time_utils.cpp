#include "time_utils.h"

#include <cstdio>

// Days since 1970-01-01 for a UTC civil (y/m/d) date. Pure integer arithmetic,
// no timezone/DST involved at all - civil_from_days algorithm, Howard Hinnant,
// http://howardhinnant.github.io/date_algorithms.html
static long long daysFromCivilUTC(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  long long era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (long long)doe - 719468;
}

// TfL sends UTC timestamps. mktime()/gmtime() go through the current TZ rules
// and only produce a correct UTC epoch here by a coincidence of London's
// standard offset being 0 - it breaks for any TZ where it isn't. This does
// the UTC civil-date-to-epoch conversion directly instead, so it's correct
// regardless of TZ, DST state, or libc mktime() quirks.
time_t parseISO8601UTC(const char* isoTime) {
  if (!isoTime || strlen(isoTime) < 19) return 0;

  int year, month, day, hour, minute, second;
  if (sscanf(isoTime, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
    return 0;
  }

  long long days = daysFromCivilUTC(year, (unsigned)month, (unsigned)day);
  return (time_t)(days * 86400LL + hour * 3600 + minute * 60 + second);
}
