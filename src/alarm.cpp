#include "alarm.h"

#include <stdio.h>

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "alarm_logic.h"
#include "rtos_objects.h"
#include "serial_log.h"
#include "system_state.h"

/* TIM1 counts at kPwmTickHz; one PWM period is kPwmTickHz / kBuzzerHz ticks. */
static constexpr uint32_t kPwmTickHz = 1000000;
static constexpr uint32_t kBuzzerHz = 1000;

/* The alarm normally wakes on each sample (every 2 s). This timeout makes it
 * also re-check EVENT_ACTIVE when no samples arrive, so the buzzer is
 * silenced promptly if the system goes INACTIVE. */
static constexpr uint32_t kActiveRecheckMs = 500;

static TIM_HandleTypeDef htim1;

/* TIM1 is on APB2. Its clock is PCLK2 when the APB2 prescaler is 1, and
 * 2 x PCLK2 otherwise (RM0008, clock tree). */
static uint32_t tim1_clock_hz(void) {
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
    bool apb2_undivided = (RCC->CFGR & RCC_CFGR_PPRE2) == RCC_CFGR_PPRE2_DIV1;
    return apb2_undivided ? pclk2 : 2U * pclk2;
}

void alarm_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PA8 Buzzer: TIM1_CH1 alternate function
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    uint32_t timer_hz = tim1_clock_hz();
    uint32_t prescaler = timer_hz / kPwmTickHz - 1;
    uint32_t period = kPwmTickHz / kBuzzerHz - 1;

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = prescaler;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = period;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim1);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = (period + 1) / 2;                  /* 50 % duty */
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    oc.OCIdleState = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_1);

    char line[80];
    snprintf(line, sizeof(line), "ALARM: TIM1 clock %lu Hz, PSC=%lu, ARR=%lu -> %lu Hz PWM",
             (unsigned long)timer_hz, (unsigned long)prescaler, (unsigned long)period,
             (unsigned long)(timer_hz / (prescaler + 1) / (period + 1)));
    log_line(line);
}

/* HAL_TIM_PWM_Start also sets TIM1's main-output enable (MOE), which the
 * advanced-control timer needs before any output reaches the pin. */
static void buzzer_set(bool on) {
    if (on) {
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    } else {
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    }
}

void AlarmTask(void *pvParameters) {
    AlarmState state = AlarmState::NORMAL;
    bool sounding = false;
    char line[64];

    log_line("AlarmTask started");

    for (;;) {
        SensorData sample;
        if (xQueueReceive(xAlarmQueue, &sample, pdMS_TO_TICKS(kActiveRecheckMs)) == pdTRUE) {
            AlarmState new_state = evaluateTemperature(sample.temperature);
            if (new_state != state) {
                state = new_state;
                snprintf(line, sizeof(line), "ALARM: temperature %s", alarmStateName(state));
                log_line(line);
            }
        }

        /* FR-07 alarm, only while ACTIVE (FR-08). */
        bool active = (xEventGroupGetBits(xSystemEvents) & EVENT_ACTIVE) != 0;
        bool should_sound = active && state != AlarmState::NORMAL;

        if (should_sound != sounding) {
            sounding = should_sound;
            buzzer_set(sounding);
            if (sounding) {
                xEventGroupSetBits(xSystemEvents, EVENT_ALARM);
            } else {
                xEventGroupClearBits(xSystemEvents, EVENT_ALARM);
            }
            log_line(sounding ? "ALARM: buzzer on" : "ALARM: buzzer off");
        }
    }
}
