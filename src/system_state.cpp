#include "system_state.h"

#include "FreeRTOS.h"
#include "task.h"
#include "rtos_objects.h"
#include "serial_log.h"

/* EVENT_MOTION is a level bit that stays set while the PIR sees motion, so
 * waiting on it would return immediately again and again. After seeing
 * motion, StateTask sleeps this long before waiting again. */
static constexpr uint32_t kMotionHoldOffMs = 500;

void StateTask(void *pvParameters) {
    SystemState state = SystemState::ACTIVE;    /* boots ACTIVE (EVENT_ACTIVE set at creation) */
    const TickType_t timeout = pdMS_TO_TICKS(kInactivityTimeoutMs);
    TickType_t last_motion = xTaskGetTickCount();

    log_line("StateTask started");

    for (;;) {
        /* ACTIVE: wait for motion, but only until the 15 s since the last
         * motion run out. INACTIVE: only motion can change anything. */
        TickType_t since = xTaskGetTickCount() - last_motion;
        TickType_t wait = portMAX_DELAY;
        if (state == SystemState::ACTIVE) {
            wait = since < timeout ? timeout - since : 0;
        }

        EventBits_t bits = xEventGroupWaitBits(xSystemEvents, EVENT_MOTION,
                                               pdFALSE,   /* MotionTask owns the bit */
                                               pdFALSE, wait);
        bool motion = (bits & EVENT_MOTION) != 0;

        TickType_t now = xTaskGetTickCount();
        if (motion) {
            last_motion = now;
        }
        bool timeout_elapsed = (now - last_motion) >= timeout;

        SystemState next = evaluateSystemState(state, motion, timeout_elapsed);
        if (next != state) {
            state = next;
            if (state == SystemState::ACTIVE) {
                xEventGroupSetBits(xSystemEvents, EVENT_ACTIVE);
                log_line("STATE: ACTIVE (motion)");
            } else {
                xEventGroupClearBits(xSystemEvents, EVENT_ACTIVE);
                log_line("STATE: INACTIVE (no motion for 15 s)");
            }
        }

        if (motion) {
            vTaskDelay(pdMS_TO_TICKS(kMotionHoldOffMs));
        }
    }
}
