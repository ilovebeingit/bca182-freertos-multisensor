#include "input.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "input_logic.h"
#include "system_state.h"

void input_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PA3 (CLK), PA4 (DT), PA5 (SW) Encoder
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void InputTask(void *pvParameters) {
    bool prev_clk = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET;

    for (;;) {
        bool clk = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET;
        bool dt = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET;

        int8_t step = encoder_step(prev_clk, clk, dt);
        if (step != 0) {
            g_encoder_count += step;
        }
        prev_clk = clk;

        bool sw = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET;
        g_button_flag = button_is_pressed(sw) ? 1 : 0;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
