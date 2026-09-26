#pragma once

/* Shared application state. Hardware-independent: no HAL or FreeRTOS headers,
 * so it can be compiled for platform=native unit tests. */

/* One SensorTask sample (CLAUDE.md), sent by value through xDisplayQueue and
 * xAlarmQueue. SensorTask only sends a sample after a successful DHT22 read,
 * so temperature and humidity are always real measurements. */
struct SensorData {
    float temperature;      /* degC */
    float humidity;         /* %RH */
    int lightLevel;         /* relative ambient light, 0-100 % (not lux) */
    bool motionDetected;    /* EVENT_MOTION when the sample was taken */
};
