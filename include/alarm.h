#pragma once

/* Passive buzzer on PA8, driven by TIM1_CH1 PWM at about 1 kHz. */

/* Sets up TIM1_CH1 PWM (output stopped). The prescaler is computed from the
 * real TIM1 clock, and the result is logged. */
void alarm_init(void);

/* Runs on every sensor update from xAlarmQueue (FR-07): evaluates the
 * temperature and sounds the buzzer while it is outside 18.0-30.0 degC and
 * the system is ACTIVE. Sets EVENT_ALARM while sounding, clears it otherwise. */
void AlarmTask(void *pvParameters);
