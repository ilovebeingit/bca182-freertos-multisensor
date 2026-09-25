#include "motion.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "motion_logic.h"
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
    for (;;) {
        bool out = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_SET;
        g_motion_flag = pir_motion_detected(out) ? 1 : 0;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
