#pragma once

/* PIR sensor output on PA2. */

void motion_init(void);

/* Samples the PIR every 100 ms into g_motion_flag. */
void MotionTask(void *pvParameters);
