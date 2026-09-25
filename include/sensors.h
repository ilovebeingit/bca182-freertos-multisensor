#pragma once

/* DHT22 on PA0, LDR on PA1 (ADC1 channel 1) and the PC13 heartbeat LED. */

void sensors_init(void);

/* Every 2000 ms (vTaskDelayUntil): read the DHT22 and LDR, toggle the
 * heartbeat LED, overwrite the sample into xDisplayQueue and xAlarmQueue and
 * log it. */
void SensorTask(void *pvParameters);
