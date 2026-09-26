#include "alarm_logic.h"

AlarmState evaluateTemperature(float temperatureC) {
    if (temperatureC < kTempLowLimitC) {
        return AlarmState::LOW_TEMPERATURE;
    }
    if (temperatureC > kTempHighLimitC) {
        return AlarmState::HIGH_TEMPERATURE;
    }
    return AlarmState::NORMAL;
}

const char *alarmStateName(AlarmState state) {
    switch (state) {
        case AlarmState::NORMAL:           return "NORMAL";
        case AlarmState::LOW_TEMPERATURE:  return "LOW";
        case AlarmState::HIGH_TEMPERATURE: return "HIGH";
    }
    return "";
}
