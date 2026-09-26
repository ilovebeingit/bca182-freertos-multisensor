#include "input.h"

#include <stdio.h>

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "display_logic.h"
#include "input_logic.h"
#include "rtos_objects.h"
#include "serial_log.h"

static constexpr uint32_t kInputPeriodMs = 5;

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
    /* Encoder decoder state and the navigation state live only here. */
    bool prev_clk = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET;
    bool prev_pressed = false;
    DisplayMode mode = kInitialDisplayMode;
    char line[32];

    log_line("InputTask started");

    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(kInputPeriodMs));

        bool clk = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET;
        bool dt = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET;

        /* The encoder is decoded all the time, so no false step appears when
         * the system wakes up, but it only changes the mode while ACTIVE. */
        int8_t step = encoder_step(prev_clk, clk, dt);
        prev_clk = clk;
        bool active = (xEventGroupGetBits(xSystemEvents) & EVENT_ACTIVE) != 0;
        if (step != 0 && active) {
            /* +1 is clockwise (next), -1 counterclockwise (previous). */
            mode = step > 0 ? nextDisplayMode(mode) : previousDisplayMode(mode);
            xQueueOverwrite(xModeQueue, &mode);

            snprintf(line, sizeof(line), "INPUT: mode %s", display_mode_label(mode));
            log_line(line);
        }

        bool sw = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET;
        bool pressed = button_is_pressed(sw);
        if (pressed && !prev_pressed) {
            log_line("INPUT: button pressed");
        }
        prev_pressed = pressed;
    }
}
