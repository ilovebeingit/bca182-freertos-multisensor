#include "motion.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "motion_logic.h"
#include "serial_log.h"
#include "system_state.h"

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

    for (;;) {
        bool out = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_SET;
        bool detected = pir_motion_detected(out);
        g_motion_flag = detected ? 1 : 0;
        if (detected != prev_detected) {
            log_line(detected ? "MOTION: detected" : "MOTION: clear");
        }
        prev_detected = detected;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
