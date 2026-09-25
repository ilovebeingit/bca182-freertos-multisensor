#include "rtos_objects.h"

#include "system_state.h"

QueueHandle_t xSensorQueue = NULL;
SemaphoreHandle_t serialMutex = NULL;

bool rtos_objects_create(void) {
    xSensorQueue = xQueueCreate(1, sizeof(RoomData_t));
    serialMutex = xSemaphoreCreateMutex();
    return xSensorQueue != NULL && serialMutex != NULL;
}
