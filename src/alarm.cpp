#include "alarm.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "alarm_logic.h"
#include "rtos_objects.h"
#include "serial_log.h"
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
    bool prev_sounding = false;
    SensorData_t latest = {0.0f, 0.0f, 0, false};

    log_line("AlarmTask started");

    for (;;) {
        /* Take the newest sample if one arrived, without blocking: the buzzer
         * timing below must not wait on the 2 s sensor period. `latest` is
         * the input for the threshold/state-machine stage; the buzzer itself
         * still follows motion only. */
        xQueueReceive(xAlarmQueue, &latest, 0);

        bool sounding = alarm_should_sound(g_motion_flag);
        if (sounding != prev_sounding) {
            log_line(sounding ? "ALARM: buzzer on" : "ALARM: buzzer off");
        }
        prev_sounding = sounding;

        if (sounding) {
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
