#include "serial_log.h"

#include <string.h>

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "rtos_objects.h"

static constexpr uint32_t kUartTimeoutMs = 100;

static UART_HandleTypeDef huart1;

void serial_log_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PA9 USART1 TX
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

void log_line(const char *msg) {
    bool use_mutex = xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED;

    if (use_mutex) {
        xSemaphoreTake(serialMutex, portMAX_DELAY);
    }

    HAL_UART_Transmit(&huart1, (const uint8_t *)msg, (uint16_t)strlen(msg), kUartTimeoutMs);
    HAL_UART_Transmit(&huart1, (const uint8_t *)"\r\n", 2, kUartTimeoutMs);

    if (use_mutex) {
        xSemaphoreGive(serialMutex);
    }
}
