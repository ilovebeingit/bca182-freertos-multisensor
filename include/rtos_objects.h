#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

/* Latest sensor sample, length 1: SensorTask overwrites, DisplayTask receives. */
extern QueueHandle_t xSensorQueue;

/* Guards USART1: held by log_line() for one whole line at a time. */
extern SemaphoreHandle_t serialMutex;

/* Creates the kernel objects above. Returns false if any allocation failed. */
bool rtos_objects_create(void);
