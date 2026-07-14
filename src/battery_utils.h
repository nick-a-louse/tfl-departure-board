#ifndef BATTERY_UTILS_H
#define BATTERY_UTILS_H

#include <Arduino.h>

// Reads the battery voltage in millivolts, using the ESP32's factory ADC
// calibration rather than a raw linear formula.
uint32_t readBatteryVoltageMv();

// Converts a voltage reading to an estimated percentage remaining, using a
// sigmoidal approximation of the LiPo discharge curve.
int calcBatteryPercent(uint32_t mv);

#endif
