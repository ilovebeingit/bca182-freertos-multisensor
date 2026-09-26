#pragma once

/* Passive buzzer on PA8. */

void alarm_init(void);

/* Beeps in bursts while EVENT_MOTION is set; silent otherwise. Also keeps
 * the newest sample from xAlarmQueue for the upcoming threshold logic. */
void AlarmTask(void *pvParameters);
