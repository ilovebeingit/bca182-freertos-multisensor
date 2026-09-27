#pragma once

/* Shared application state and the ACTIVE/INACTIVE decision.
 * Hardware-independent: no HAL or FreeRTOS headers, so the pure parts
 * (SensorData, SystemState, evaluateSystemState in system_state_logic.cpp)
 * can be compiled for platform=native unit tests. StateTask itself is in
 * system_state.cpp. */

#include <stdint.h>

/* One SensorTask sample (docs/requirements.md), sent by value through xDisplayQueue and
 * xAlarmQueue. SensorTask only sends a sample after a successful DHT22 read,
 * so temperature and humidity are always real measurements. */
struct SensorData {
    float temperature;      /* degC */
    float humidity;         /* %RH */
    int lightLevel;         /* relative ambient light, 0-100 % (not lux) */
    bool motionDetected;    /* EVENT_MOTION when the sample was taken */
};

/* FR-08: the system is ACTIVE (OLED on, sensing, encoder and alarm active) or
 * INACTIVE (OLED blank, PIR still monitored). */
enum class SystemState {
    ACTIVE,
    INACTIVE,
};

/* FR-09: this long without motion makes an ACTIVE system INACTIVE. */
constexpr uint32_t kInactivityTimeoutMs = 15000;

/* Next state. Motion always gives ACTIVE (FR-10), even if the timeout has also
 * elapsed; otherwise ACTIVE becomes INACTIVE once timeoutElapsed (FR-09), and
 * INACTIVE stays INACTIVE. */
SystemState evaluateSystemState(SystemState current, bool motionDetected, bool timeoutElapsed);

/* Runs the state machine on EVENT_MOTION with the 15 s timeout, and owns
 * EVENT_ACTIVE (set while ACTIVE, clear while INACTIVE). */
void StateTask(void *pvParameters);
