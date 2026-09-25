#include "sensors.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rtos_objects.h"
#include "system_state.h"

static ADC_HandleTypeDef hadc1;

void sensors_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PC13 LED
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // PA1 LDR analog input
    GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

void SensorTask(void *pvParameters) {
    RoomData_t sensorData = {24.5f, 60.0f, 0, 0, 0, 0};

    for (;;) {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            sensorData.light_level = (uint16_t)HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);

        sensorData.motion_detected = g_motion_flag;
        sensorData.encoder_count = g_encoder_count;
        sensorData.button_pressed = g_button_flag;

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        xQueueOverwrite(xSensorQueue, &sensorData);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
