#include "alarm.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "alarm_logic.h"
#include "system_state.h"

void alarm_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PA8 Buzzer Output
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void AlarmTask(void *pvParameters) {
    for (;;) {
        if (alarm_should_sound(g_motion_flag)) {
            for (uint32_t i = 0; i < kAlarmBurstCycles; i++) {
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
                vTaskDelay(pdMS_TO_TICKS(kAlarmHalfPeriodMs));
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
                vTaskDelay(pdMS_TO_TICKS(kAlarmHalfPeriodMs));
            }
            vTaskDelay(pdMS_TO_TICKS(kAlarmBurstPauseMs));
        } else {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
            vTaskDelay(pdMS_TO_TICKS(kAlarmIdlePollMs));
        }
    }
}
