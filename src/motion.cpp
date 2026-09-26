#include "motion.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "motion_logic.h"
#include "rtos_objects.h"
#include "serial_log.h"

static constexpr uint32_t kMotionPeriodMs = 100;

void motion_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PA2 PIR Input
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void MotionTask(void *pvParameters) {
    bool prev_detected = false;

    log_line("MotionTask started");

    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(kMotionPeriodMs));

        bool out = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_SET;
        bool detected = pir_motion_detected(out);

        /* EVENT_MOTION mirrors the PIR level; only this task writes it. */
        if (detected) {
            xEventGroupSetBits(xSystemEvents, EVENT_MOTION);
        } else {
            xEventGroupClearBits(xSystemEvents, EVENT_MOTION);
        }

        if (detected != prev_detected) {
            log_line(detected ? "MOTION: detected" : "MOTION: clear");
        }
        prev_detected = detected;
    }
}
