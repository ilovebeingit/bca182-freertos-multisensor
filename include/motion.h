#pragma once

/* PIR sensor output on PA2. */

void motion_init(void);

/* Every 100 ms (vTaskDelayUntil): samples the PIR and sets or clears
 * EVENT_MOTION in xSystemEvents to match it. */
void MotionTask(void *pvParameters);
