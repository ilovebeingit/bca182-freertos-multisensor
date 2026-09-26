#pragma once

/* SSD1306 128x64 OLED on I2C1 (PB6 SCL, PB7 SDA), address 0x3C.
 * DisplayTask is the only code that talks to the panel. */

/* Sets up I2C1 only; the panel itself is initialised by DisplayTask. */
void display_init(void);

/* Initialises the panel, then blocks on xDisplayEvents and redraws the
 * current mode's screen whenever a new sample (xDisplayQueue, every 2 s), a
 * new mode (xModeQueue, from the encoder) or an EVENT_MOTION / EVENT_ALARM
 * change arrives. While EVENT_ACTIVE is clear the OLED is blank. */
void DisplayTask(void *pvParameters);
