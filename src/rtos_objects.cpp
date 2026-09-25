#include "rtos_objects.h"

#include "system_state.h"

QueueHandle_t xDisplayQueue = NULL;
QueueHandle_t xAlarmQueue = NULL;
SemaphoreHandle_t serialMutex = NULL;

bool rtos_objects_create(void) {
    xDisplayQueue = xQueueCreate(1, sizeof(SensorData_t));
    xAlarmQueue = xQueueCreate(1, sizeof(SensorData_t));
    serialMutex = xSemaphoreCreateMutex();
    return xDisplayQueue != NULL && xAlarmQueue != NULL && serialMutex != NULL;
}
