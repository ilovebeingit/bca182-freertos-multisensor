#pragma once

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"

/* xSystemEvents: system-wide status flags, one bit each.
 *   EVENT_ACTIVE  system is ACTIVE (set at boot; the state machine will own it)
 *   EVENT_MOTION  PIR currently reports motion. MotionTask sets and clears it
 *                 every 100 ms; readers only look.
 *   EVENT_ALARM   temperature alarm is sounding (reserved for AlarmTask). */
constexpr EventBits_t EVENT_ACTIVE = (1U << 0);
constexpr EventBits_t EVENT_MOTION = (1U << 1);
constexpr EventBits_t EVENT_ALARM  = (1U << 2);
extern EventGroupHandle_t xSystemEvents;

/* Latest SensorData sample, one queue per consumer, each of length 1:
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
