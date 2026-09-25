#pragma once

/* SSD1306 128x64 OLED on I2C1 (PB6 SCL, PB7 SDA), address 0x3C.
 * DisplayTask is the only code that talks to the panel. */

/* Sets up I2C1 only; the panel itself is initialised by DisplayTask. */
void display_init(void);

/* Initialises the panel, then for every sample received from xDisplayQueue
 * (every 2 s) renders the current mode's screen and flushes the frame. */
void DisplayTask(void *pvParameters);
