#pragma once

#include "FreeRTOS.h"
#include "queue.h"

/* Latest sensor sample, length 1: SensorTask overwrites, DisplayTask receives. */
extern QueueHandle_t xSensorQueue;

/* Creates the kernel objects above. Returns false if any allocation failed. */
bool rtos_objects_create(void);
