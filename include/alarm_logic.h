#pragma once

/* Temperature alarm decision. Hardware-independent (no HAL/FreeRTOS). */

/* FR-07: the buzzer sounds when the temperature is outside this range. Both
 * limits are inside the range (18.0 and 30.0 degC are NORMAL). */
constexpr float kTempLowLimitC = 18.0f;
constexpr float kTempHighLimitC = 30.0f;

enum class AlarmState {
    NORMAL,
    LOW_TEMPERATURE,
    HIGH_TEMPERATURE,
};

AlarmState evaluateTemperature(float temperatureC);

/* Short text for the log and the OLED alarm line: "NORMAL", "LOW", "HIGH". */
const char *alarmStateName(AlarmState state);
