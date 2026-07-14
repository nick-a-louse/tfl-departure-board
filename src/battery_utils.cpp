#include "battery_utils.h"

#include <cmath>
#include <driver/adc.h>
#include <esp_adc_cal.h>

#include "config.h"

uint32_t readBatteryVoltageMv() {
  // adc_power_acquire()/release() keep the ADC peripheral powered for the
  // read - without it, a read taken shortly after wake can return a stale
  // value because the ADC was left powered down.
  adc_power_acquire();
  uint16_t adcRaw = analogRead(PIN_BAT_ADC);
  adc_power_release();

  // Use the chip's eFuse-burned calibration (Vref or two-point) instead of a
  // raw linear formula - corrects for this specific chip's ADC nonlinearity.
  esp_adc_cal_characteristics_t adcChars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_11db, ADC_WIDTH_BIT_12, 1100, &adcChars);
  uint32_t pinMv = esp_adc_cal_raw_to_voltage(adcRaw, &adcChars);

  return (uint32_t)(pinMv * BATTERY_DIVIDER_RATIO);
}

// Symmetric sigmoidal approximation of a LiPo discharge curve - a straight
// voltage-to-percentage line reads wrong because LiPo voltage sags fast near
// empty and stays fairly flat through the middle of the discharge curve.
// Formula and constants adapted from https://github.com/rlogiacco/BatterySense
// (LGPL-3.0), validated on the same FireBeetle 2 ESP32-E hardware in the
// sibling Weather_Picture_Frame project.
int calcBatteryPercent(uint32_t mv) {
  if (mv <= MIN_BATTERY_VOLTAGE_MV) return 0;
  if (mv >= MAX_BATTERY_VOLTAGE_MV) return 100;

  double v = mv;
  double minv = MIN_BATTERY_VOLTAGE_MV;
  double maxv = MAX_BATTERY_VOLTAGE_MV;
  double pct = 105.0 - (105.0 / (1.0 + pow(1.724 * (v - minv) / (maxv - minv), 5.5)));

  if (pct > 100.0) pct = 100.0;
  if (pct < 0.0) pct = 0.0;
  return (int)(pct + 0.5);
}
