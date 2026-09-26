#pragma once

/* Sensor value conversions. Hardware-independent (no HAL/FreeRTOS). */

#include <stdint.h>

/* Full scale of the 12-bit ADC. */
constexpr uint16_t kAdcMax = 4095;

/* Converts the LDR module's raw ADC reading to relative light, 0-100 %.
 * The module's AO voltage falls as light rises (assumption, to be confirmed
 * in Wokwi; see docs/dev-log.md), so 0 -> 100 % and kAdcMax -> 0 %. Rounded
 * to the nearest percent; readings above kAdcMax are treated as kAdcMax. */
int lightPercentFromAdc(uint16_t raw);
