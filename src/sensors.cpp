#include "sensors.h"

#include <stdio.h>

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "dht22.h"
#include "rtos_objects.h"
#include "serial_log.h"
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

    // PA0 DHT22
    dht22_init();
}

/* One sample per period. 2000 ms is also the DHT22's minimum read interval. */
static constexpr uint32_t kSensorPeriodMs = 2000;
static_assert(kSensorPeriodMs >= kDht22MinIntervalMs,
              "the DHT22 must not be read more often than every 2 s");

/* Returns the LDR ADC value, or `previous` if the conversion times out. */
static uint16_t read_ldr(uint16_t previous) {
    uint16_t value = previous;

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);
    return value;
}

void SensorTask(void *pvParameters) {
    SensorData_t sample = {0.0f, 0.0f, 0, false};
    char line[80];
    char temperature[12];
    char humidity[12];

    log_line("SensorTask started");

    /* The first sample is taken one period after start, which also covers the
     * DHT22's power-up settling time. */
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(kSensorPeriodMs));

        /* dht22_read writes `reading` only on success, so a failed read leaves
         * the previous temperature/humidity in `sample`, flagged invalid. */
        Dht22Reading reading;
        Dht22Status status = dht22_read(&reading);
        sample.dht_valid = (status == DHT22_OK);
        if (sample.dht_valid) {
            sample.temperature = reading.temperature_tenths / 10.0f;
            sample.humidity = reading.humidity_tenths / 10.0f;
        }
        sample.light_level = read_ldr(sample.light_level);

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        xQueueOverwrite(xDisplayQueue, &sample);
        xQueueOverwrite(xAlarmQueue, &sample);

        if (sample.dht_valid) {
            format_tenths_2dp(temperature, sizeof(temperature), reading.temperature_tenths);
            format_tenths_2dp(humidity, sizeof(humidity), reading.humidity_tenths);
            snprintf(line, sizeof(line), "Sample: Temperature: %s C, Humidity: %s %%, Light: %u",
                     temperature, humidity, sample.light_level);
        } else {
            snprintf(line, sizeof(line), "DHT22: read failed (%s), Light: %u",
                     dht22_status_name(status), sample.light_level);
        }
        log_line(line);
    }
}
