#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

#define SSD1306_I2C_ADDR (0x3C << 1)

static const uint8_t Font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00,
    0x00, 0x07, 0x00, 0x07, 0x00, 0x14, 0x7F, 0x14, 0x7F, 0x14,
    0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62,
    0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x05, 0x03, 0x00, 0x00,
    0x00, 0x1C, 0x22, 0x41, 0x00, 0x00, 0x41, 0x22, 0x1C, 0x00,
    0x14, 0x08, 0x3E, 0x08, 0x14, 0x08, 0x08, 0x3E, 0x08, 0x08,
    0x00, 0x50, 0x30, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x00, 0x60, 0x60, 0x00, 0x00, 0x20, 0x10, 0x08, 0x04, 0x02,
    0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x42, 0x7F, 0x40, 0x00,
    0x42, 0x61, 0x51, 0x49, 0x46, 0x21, 0x41, 0x45, 0x4B, 0x31,
    0x18, 0x14, 0x12, 0x7F, 0x10, 0x27, 0x45, 0x45, 0x45, 0x39,
    0x3C, 0x4A, 0x49, 0x49, 0x30, 0x01, 0x71, 0x09, 0x05, 0x03,
    0x36, 0x49, 0x49, 0x49, 0x36, 0x06, 0x49, 0x49, 0x29, 0x1E,
    0x00, 0x36, 0x36, 0x00, 0x00, 0x00, 0x56, 0x36, 0x00, 0x00,
    0x08, 0x14, 0x22, 0x41, 0x00, 0x14, 0x14, 0x14, 0x14, 0x14,
    0x00, 0x41, 0x22, 0x14, 0x08, 0x02, 0x01, 0x51, 0x09, 0x06,
    0x32, 0x49, 0x79, 0x41, 0x3E, 0x7E, 0x11, 0x11, 0x11, 0x7E,
    0x7F, 0x49, 0x49, 0x49, 0x36, 0x3E, 0x41, 0x41, 0x41, 0x22,
    0x7F, 0x41, 0x41, 0x22, 0x1C, 0x7F, 0x49, 0x49, 0x49, 0x41,
    0x7F, 0x09, 0x09, 0x09, 0x01, 0x3E, 0x41, 0x49, 0x49, 0x7A,
    0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x41, 0x7F, 0x41, 0x00,
    0x20, 0x40, 0x41, 0x3F, 0x01, 0x7F, 0x08, 0x14, 0x22, 0x41,
    0x7F, 0x40, 0x40, 0x40, 0x40, 0x7F, 0x02, 0x0C, 0x02, 0x7F,
    0x7F, 0x04, 0x08, 0x10, 0x7F, 0x3E, 0x41, 0x41, 0x41, 0x3E,
    0x7F, 0x09, 0x09, 0x09, 0x06, 0x3E, 0x41, 0x51, 0x21, 0x5E,
    0x7F, 0x09, 0x19, 0x29, 0x46, 0x46, 0x49, 0x49, 0x49, 0x31,
    0x01, 0x01, 0x7F, 0x01, 0x01, 0x3F, 0x40, 0x40, 0x40, 0x3F,
    0x1F, 0x20, 0x40, 0x20, 0x1F, 0x3F, 0x40, 0x38, 0x40, 0x3F,
    0x63, 0x14, 0x08, 0x14, 0x63, 0x07, 0x08, 0x70, 0x08, 0x07,
    0x61, 0x51, 0x49, 0x45, 0x43
};

static uint8_t SSD1306_Buffer[1024];

typedef struct {
    float temperature;
    float humidity;
    uint16_t light_level;
    uint8_t motion_detected;
    int32_t encoder_count;
    uint8_t button_pressed;
} RoomData_t;

QueueHandle_t xSensorQueue = NULL;
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
void SSD1306_Init(void);
void SSD1306_SendCmd(uint8_t cmd);
void SSD1306_UpdateScreen(void);
void SSD1306_Clear(void);
void SSD1306_DrawChar(uint8_t x, uint8_t y, char c);
void SSD1306_WriteString(uint8_t x, uint8_t y, const char *str);

void SensorTask(void *pvParameters);
void DisplayTask(void *pvParameters);
void InputTask(void *pvParameters);
void MotionTask(void *pvParameters);
void AlarmTask(void *pvParameters);

volatile uint8_t g_motion_flag = 0;
volatile int32_t g_encoder_count = 0;
volatile uint8_t g_button_flag = 0;

extern void xPortSysTickHandler(void);

/* HAL_Init() starts SysTick long before the scheduler exists. Always advance
 * the HAL tick (uwTick, which the default HAL_GetTick reads) so HAL timeouts
 * work before and after the scheduler starts, and only hand the tick to
 * FreeRTOS once the kernel is running. */
void SysTick_Handler(void) {
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}

int main(void) {
    /* SystemInit() leaves VTOR at its reset value 0. FreeRTOS reads the initial
     * MSP from *VTOR when starting the first task, which only works where 0x0
     * aliases flash, so point VTOR at the flash vector table explicitly. */
    SCB->VTOR = FLASH_BASE;
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();

    xSensorQueue = xQueueCreate(1, sizeof(RoomData_t));

    if (xSensorQueue != NULL) {
        xTaskCreate(SensorTask,  "SensorTask",  256, NULL, 2, NULL);
        xTaskCreate(DisplayTask, "DisplayTask", 256, NULL, 1, NULL);
        xTaskCreate(InputTask,   "InputTask",   128, NULL, 3, NULL);
        xTaskCreate(MotionTask,  "MotionTask",  128, NULL, 3, NULL);
        xTaskCreate(AlarmTask,   "AlarmTask",   128, NULL, 2, NULL);

        vTaskStartScheduler();
    }

    while (1) {}
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

void DisplayTask(void *pvParameters) {
    RoomData_t rxData;
    char lineBuf[24];

    SSD1306_Init();

    for (;;) {
        if (xQueueReceive(xSensorQueue, &rxData, portMAX_DELAY) == pdTRUE) {
            SSD1306_Clear();

            SSD1306_WriteString(0, 0,  "RTOS DASHBOARD");

            snprintf(lineBuf, sizeof(lineBuf), "LIGHT: %u", rxData.light_level);
            SSD1306_WriteString(0, 16, lineBuf);

            snprintf(lineBuf, sizeof(lineBuf), "MOTION: %s", rxData.motion_detected ? "DETECTED" : "CLEAR");
            SSD1306_WriteString(0, 32, lineBuf);

            snprintf(lineBuf, sizeof(lineBuf), "ENCODER: %ld", (long)rxData.encoder_count);
            SSD1306_WriteString(0, 48, lineBuf);

            SSD1306_UpdateScreen();
        }
    }
}

void InputTask(void *pvParameters) {
    GPIO_PinState prev_clk = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3);

    for (;;) {
        GPIO_PinState curr_clk = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3);

        if (prev_clk == GPIO_PIN_SET && curr_clk == GPIO_PIN_RESET) {
            if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) != curr_clk) {
                g_encoder_count++;
            } else {
                g_encoder_count--;
            }
        }
        prev_clk = curr_clk;

        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_RESET) {
            g_button_flag = 1;
        } else {
            g_button_flag = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void MotionTask(void *pvParameters) {
    for (;;) {
        GPIO_PinState motionState = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2);
        g_motion_flag = (motionState == GPIO_PIN_SET) ? 1 : 0;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void AlarmTask(void *pvParameters) {
    for (;;) {
        if (g_motion_flag) {
            for (int i = 0; i < 20; i++) {
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
                vTaskDelay(pdMS_TO_TICKS(2));
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
                vTaskDelay(pdMS_TO_TICKS(2));
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

void SSD1306_SendCmd(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};
    HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, data, 2, 100);
}

void SSD1306_Init(void) {
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

void SSD1306_Clear(void) {
    memset(SSD1306_Buffer, 0x00, sizeof(SSD1306_Buffer));
}

void SSD1306_UpdateScreen(void) {
    uint8_t chunk[129];
    chunk[0] = 0x40;

    for (uint8_t i = 0; i < 8; i++) {
        SSD1306_SendCmd(0xB0 + i);
        SSD1306_SendCmd(0x00);
        SSD1306_SendCmd(0x10);

        memcpy(&chunk[1], &SSD1306_Buffer[128 * i], 128);
        HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, chunk, 129, 100);
    }
}

void SSD1306_DrawChar(uint8_t x, uint8_t y, char c) {
    if (c < ' ' || c > 'Z') c = ' ';
    uint16_t font_idx = (c - ' ') * 5;

    for (uint8_t i = 0; i < 5; i++) {
        if (x + i >= 128) break;
        uint8_t line = Font5x7[font_idx + i];
        uint8_t page = y / 8;
        if (page < 8) {
            SSD1306_Buffer[x + i + (page * 128)] = line;
        }
    }
}

void SSD1306_WriteString(uint8_t x, uint8_t y, const char *str) {
    while (*str) {
        SSD1306_DrawChar(x, y, *str);
        x += 6;
        str++;
    }
}

static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PC13 LED
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // PA2 PIR Input
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA8 Buzzer Output
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA3 (CLK), PA4 (DT), PA5 (SW) Encoder
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_I2C1_Init(void) {
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

static void MX_ADC1_Init(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
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

void SystemClock_Config(void) {}
