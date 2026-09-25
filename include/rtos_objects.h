#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

/* Latest SensorData_t sample, one queue per consumer, each of length 1:
 * SensorTask overwrites both every period, each consumer receives from its
 * own, so neither consumer can take a sample away from the other. */
extern QueueHandle_t xDisplayQueue;
extern QueueHandle_t xAlarmQueue;

/* Latest DisplayMode chosen with the encoder, length 1: InputTask overwrites,
 * DisplayTask receives. Only the newest mode matters. */
extern QueueHandle_t xModeQueue;

/* Queue set holding xDisplayQueue and xModeQueue, so DisplayTask can block on
 * both at once and redraw as soon as either a sample or a mode change arrives. */
extern QueueSetHandle_t xDisplayEvents;

/* Guards USART1: held by log_line() for one whole line at a time. */
extern SemaphoreHandle_t serialMutex;

/* Creates the kernel objects above. Returns false if any allocation failed. */
bool rtos_objects_create(void);
