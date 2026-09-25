#include "rtos_objects.h"

#include "display_logic.h"
#include "system_state.h"

QueueHandle_t xDisplayQueue = NULL;
QueueHandle_t xAlarmQueue = NULL;
QueueHandle_t xModeQueue = NULL;
QueueSetHandle_t xDisplayEvents = NULL;
SemaphoreHandle_t serialMutex = NULL;

bool rtos_objects_create(void) {
    xDisplayQueue = xQueueCreate(1, sizeof(SensorData_t));
    xAlarmQueue = xQueueCreate(1, sizeof(SensorData_t));
    xModeQueue = xQueueCreate(1, sizeof(DisplayMode));
    serialMutex = xSemaphoreCreateMutex();

    /* A set must hold as many events as its members can hold items: 1 + 1.
     * Members must be empty when added, which they are here. */
    xDisplayEvents = xQueueCreateSet(2);

    if (xDisplayQueue == NULL || xAlarmQueue == NULL || xModeQueue == NULL ||
        serialMutex == NULL || xDisplayEvents == NULL) {
        return false;
    }
    return xQueueAddToSet(xDisplayQueue, xDisplayEvents) == pdPASS &&
           xQueueAddToSet(xModeQueue, xDisplayEvents) == pdPASS;
}
