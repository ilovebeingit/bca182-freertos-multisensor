#pragma once

/* SSD1306 128x64 OLED on I2C1 (PB6 SCL, PB7 SDA), address 0x3C. */

/* Sets up I2C1 only; the panel itself is initialised by DisplayTask. */
void display_init(void);

/* Initialises the panel, then redraws the dashboard for every sample
 * received from xSensorQueue. */
void DisplayTask(void *pvParameters);
