#include "display.h"

#include <string.h>

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "alarm_logic.h"
#include "display_logic.h"
#include "rtos_objects.h"
#include "serial_log.h"
#include "system_state.h"

/* SSD1306 over I2C: 7-bit address 0x3C, shifted for the HAL. The first byte
 * of every transfer is a control byte: 0x00 = the rest are commands,
 * 0x40 = the rest are GDDRAM data. */
#define SSD1306_I2C_ADDR   (0x3C << 1)
#define SSD1306_CTRL_CMD   0x00
#define SSD1306_CTRL_DATA  0x40

static constexpr uint32_t kI2cTimeoutMs = 100;
static constexpr uint16_t kFlushChunk = 128;   /* data bytes per I2C transfer */

/* Only this file (and only DisplayTask, after display_init) touches I2C1 and
 * the panel. */
static I2C_HandleTypeDef hi2c1;
static uint8_t s_frame[kDisplayBufferSize];    /* 128 x 64 / 8 = 1024 bytes */

static const uint8_t kInitSequence[] = {
    0xAE,         /* display off */
    0x20, 0x00,   /* memory addressing mode: horizontal */
    0xB0,         /* page start 0 (page addressing mode only) */
    0xC8,         /* COM scan remapped: row 0 at the top */
    0x00, 0x10,   /* column start 0 (page addressing mode only) */
    0x40,         /* display start line 0 */
    0x81, 0xFF,   /* contrast */
    0xA1,         /* segment remap: column 0 at the left */
    0xA6,         /* normal (not inverted) */
    0xA8, 0x3F,   /* multiplex ratio: 64 rows */
    0xA4,         /* display follows GDDRAM */
    0xD3, 0x00,   /* display offset 0 */
    0xD5, 0xF0,   /* clock divide / oscillator */
    0xD9, 0x22,   /* pre-charge period */
    0xDA, 0x12,   /* COM pins: alternative config, for 128x64 */
    0xDB, 0x20,   /* VCOMH deselect level */
    0x8D, 0x14,   /* charge pump on */
    0xAF,         /* display on */
};

static void ssd1306_command(uint8_t cmd) {
    uint8_t data[2] = {SSD1306_CTRL_CMD, cmd};
    HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, data, sizeof(data), kI2cTimeoutMs);
}

static void ssd1306_init(void) {
    for (uint16_t i = 0; i < sizeof(kInitSequence); i++) {
        ssd1306_command(kInitSequence[i]);
    }
}

/* Sends the whole frame buffer. In horizontal addressing mode the panel's
 * write pointer runs along a page, column 0..127, then wraps to the next
 * page, so after setting the window to all columns and all pages the 1024
 * buffer bytes can simply be streamed in order. */
static void ssd1306_flush(void) {
    uint8_t chunk[1 + kFlushChunk];

    ssd1306_command(0x21);                      /* column address window */
    ssd1306_command(0);
    ssd1306_command(kDisplayWidth - 1);
    ssd1306_command(0x22);                      /* page address window */
    ssd1306_command(0);
    ssd1306_command(kDisplayPages - 1);

    chunk[0] = SSD1306_CTRL_DATA;
    for (uint16_t offset = 0; offset < kDisplayBufferSize; offset += kFlushChunk) {
        memcpy(&chunk[1], &s_frame[offset], kFlushChunk);
        HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, chunk, sizeof(chunk), kI2cTimeoutMs);
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

/* Event-group bits can't join a queue set, so the set wait times out this
 * often to notice EVENT_ACTIVE / EVENT_MOTION / EVENT_ALARM changes. */
static constexpr uint32_t kEventPollMs = 250;

void DisplayTask(void *pvParameters) {
    DisplayMode mode = kInitialDisplayMode;
    SensorData sample = {0.0f, 0.0f, 0, false};
    bool have_sample = false;
    bool active = true;      /* the system boots ACTIVE */
    bool motion = false;
    bool alarm = false;

    log_line("DisplayTask started");
    ssd1306_init();
    log_line("DISPLAY: OLED initialised");

    for (;;) {
        /* Blocks until xDisplayQueue (new sample, every 2 s) or xModeQueue
         * (encoder turned) has an item and returns which one, or returns NULL
         * after kEventPollMs. Each set event corresponds to exactly one queued
         * item, so the 0-timeout receive below always succeeds. */
        QueueSetMemberHandle_t ready =
            xQueueSelectFromSet(xDisplayEvents, pdMS_TO_TICKS(kEventPollMs));
        bool changed = false;

        if (ready == xDisplayQueue) {
            if (xQueueReceive(xDisplayQueue, &sample, 0) == pdTRUE) {
                have_sample = true;
                changed = true;
            }
        } else if (ready == xModeQueue) {
            if (xQueueReceive(xModeQueue, &mode, 0) == pdTRUE) {
                changed = true;
            }
        }

        EventBits_t bits = xEventGroupGetBits(xSystemEvents);
        bool now_active = (bits & EVENT_ACTIVE) != 0;
        bool now_motion = (bits & EVENT_MOTION) != 0;
        bool now_alarm = (bits & EVENT_ALARM) != 0;
        if (now_active != active || now_motion != motion || now_alarm != alarm) {
            active = now_active;
            motion = now_motion;
            alarm = now_alarm;
            changed = true;
        }

        /* INACTIVE (FR-08): blank the OLED once, then do no display work
         * until the system is ACTIVE again. */
        if (!active) {
            if (changed) {
                display_clear(s_frame);
                ssd1306_flush();
            }
            continue;
        }

        /* Redraw only when something shown may have changed, and not before
         * the first sample, when there is nothing real to show. */
        if (have_sample && changed) {
            /* EVENT_ALARM says the buzzer is sounding; LOW vs HIGH comes from
             * the same sample AlarmTask evaluated. */
            const char *alarm_text = nullptr;
            if (alarm) {
                alarm_text = evaluateTemperature(sample.temperature) == AlarmState::LOW_TEMPERATURE
                                 ? "ALARM LOW" : "ALARM HIGH";
            }
            display_render_screen(s_frame, mode, &sample, motion, alarm_text);
            ssd1306_flush();
        }
    }
}
