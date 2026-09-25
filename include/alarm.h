#pragma once

/* Passive buzzer on PA8. */

void alarm_init(void);

/* Beeps in bursts while g_motion_flag is set; silent otherwise. Also keeps
 * the newest sample from xAlarmQueue for the upcoming threshold logic. */
void AlarmTask(void *pvParameters);
