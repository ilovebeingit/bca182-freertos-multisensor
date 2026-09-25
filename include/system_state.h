#pragma once

/* Shared application state. Hardware-independent: no HAL or FreeRTOS headers,
 * so it can be compiled for platform=native unit tests. */

#include <stdint.h>

typedef struct {
    float temperature;
    float humidity;
    uint16_t light_level;
    uint8_t motion_detected;
    int32_t encoder_count;
    uint8_t button_pressed;
} RoomData_t;

/* Written by MotionTask / InputTask, read by SensorTask and AlarmTask. */
extern volatile uint8_t g_motion_flag;
extern volatile int32_t g_encoder_count;
extern volatile uint8_t g_button_flag;
