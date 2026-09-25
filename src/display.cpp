#include "display.h"

#include <string.h>

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "display_logic.h"
#include "rtos_objects.h"
#include "system_state.h"

#define SSD1306_I2C_ADDR (0x3C << 1)

static I2C_HandleTypeDef hi2c1;
static uint8_t SSD1306_Buffer[kDisplayBufferSize];

static void SSD1306_SendCmd(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};
    HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, data, 2, 100);
}

static void SSD1306_Init(void) {
    SSD1306_SendCmd(0xAE);
    SSD1306_SendCmd(0x20);
    SSD1306_SendCmd(0x00);
    SSD1306_SendCmd(0xB0);
    SSD1306_SendCmd(0xC8);
    SSD1306_SendCmd(0x00);
    SSD1306_SendCmd(0x10);
    SSD1306_SendCmd(0x40);
    SSD1306_SendCmd(0x81);
    SSD1306_SendCmd(0xFF);
    SSD1306_SendCmd(0xA1);
    SSD1306_SendCmd(0xA6);
    SSD1306_SendCmd(0xA8);
    SSD1306_SendCmd(0x3F);
    SSD1306_SendCmd(0xA4);
    SSD1306_SendCmd(0xD3);
    SSD1306_SendCmd(0x00);
    SSD1306_SendCmd(0xD5);
    SSD1306_SendCmd(0xF0);
    SSD1306_SendCmd(0xD9);
    SSD1306_SendCmd(0x22);
    SSD1306_SendCmd(0xDA);
    SSD1306_SendCmd(0x12);
    SSD1306_SendCmd(0xDB);
    SSD1306_SendCmd(0x20);
    SSD1306_SendCmd(0x8D);
    SSD1306_SendCmd(0x14);
    SSD1306_SendCmd(0xAF);
}

static void SSD1306_UpdateScreen(void) {
    uint8_t chunk[kDisplayWidth + 1];
    chunk[0] = 0x40;

    for (uint8_t i = 0; i < kDisplayPages; i++) {
        SSD1306_SendCmd(0xB0 + i);
        SSD1306_SendCmd(0x00);
        SSD1306_SendCmd(0x10);

        memcpy(&chunk[1], &SSD1306_Buffer[kDisplayWidth * i], kDisplayWidth);
        HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, chunk, sizeof(chunk), 100);
    }
}

void display_init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    HAL_I2C_Init(&hi2c1);
}

void DisplayTask(void *pvParameters) {
    RoomData_t rxData;

    SSD1306_Init();

    for (;;) {
        if (xQueueReceive(xSensorQueue, &rxData, portMAX_DELAY) == pdTRUE) {
            display_render_dashboard(SSD1306_Buffer, &rxData);
            SSD1306_UpdateScreen();
        }
    }
}
