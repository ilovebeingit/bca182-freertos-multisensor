#pragma once

/* Passive buzzer on PA8. */

void alarm_init(void);

/* Beeps in bursts while g_motion_flag is set; silent otherwise. */
void AlarmTask(void *pvParameters);
