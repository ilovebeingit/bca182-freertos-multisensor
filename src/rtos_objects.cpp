#include "rtos_objects.h"

#include "system_state.h"

QueueHandle_t xSensorQueue = NULL;

bool rtos_objects_create(void) {
    xSensorQueue = xQueueCreate(1, sizeof(RoomData_t));
    return xSensorQueue != NULL;
}
