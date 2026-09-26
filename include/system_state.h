#pragma once

/* Shared application state. Hardware-independent: no HAL or FreeRTOS headers,
 * so it can be compiled for platform=native unit tests. */

#include <stdint.h>

/* One SensorTask sample, sent by value through xDisplayQueue and xAlarmQueue. */
typedef struct {
    float temperature;      /* degC; meaningful only when dht_valid */
    float humidity;         /* %RH;  meaningful only when dht_valid */
    uint16_t light_level;   /* raw 12-bit LDR ADC value */
    bool dht_valid;         /* true only if this sample's DHT22 read succeeded */
} SensorData_t;
