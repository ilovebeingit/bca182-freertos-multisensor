#pragma once

/* DHT22 on PA0, LDR on PA1 (ADC1 channel 1) and the PC13 heartbeat LED. */

void sensors_init(void);

/* Every 2000 ms (vTaskDelayUntil): toggle the heartbeat LED; while
 * EVENT_ACTIVE is set, read the DHT22 and LDR, overwrite the sample into
 * xDisplayQueue and xAlarmQueue and log it. A failed DHT22 read skips the
 * cycle. */
void SensorTask(void *pvParameters);
