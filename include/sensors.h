#pragma once

/* LDR on PA1 (ADC1 channel 1) and the PC13 heartbeat LED. */

void sensors_init(void);

/* Every 500 ms: sample the LDR, snapshot the shared state, toggle the
 * heartbeat LED and publish the sample to xSensorQueue. */
void SensorTask(void *pvParameters);
