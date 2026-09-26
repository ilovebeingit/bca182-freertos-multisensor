#include "rtos_objects.h"

#include "display_logic.h"
#include "system_state.h"

QueueHandle_t xDisplayQueue = NULL;
QueueHandle_t xAlarmQueue = NULL;
QueueHandle_t xModeQueue = NULL;
QueueSetHandle_t xDisplayEvents = NULL;
SemaphoreHandle_t serialMutex = NULL;
EventGroupHandle_t xSystemEvents = NULL;

bool rtos_objects_create(void) {
    xDisplayQueue = xQueueCreate(1, sizeof(SensorData));
    xAlarmQueue = xQueueCreate(1, sizeof(SensorData));
    xModeQueue = xQueueCreate(1, sizeof(DisplayMode));
    serialMutex = xSemaphoreCreateMutex();
    xSystemEvents = xEventGroupCreate();

    /* A set must hold as many events as its members can hold items: 1 + 1.
     * Members must be empty when added, which they are here. */
    xDisplayEvents = xQueueCreateSet(2);

    if (xDisplayQueue == NULL || xAlarmQueue == NULL || xModeQueue == NULL ||
        serialMutex == NULL || xDisplayEvents == NULL || xSystemEvents == NULL) {
        return false;
    }

    /* The system boots ACTIVE (FR-08). */
    xEventGroupSetBits(xSystemEvents, EVENT_ACTIVE);

    return xQueueAddToSet(xDisplayQueue, xDisplayEvents) == pdPASS &&
           xQueueAddToSet(xModeQueue, xDisplayEvents) == pdPASS;
}
