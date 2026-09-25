#pragma once

/* KY-040 rotary encoder: CLK PA3, DT PA4, SW PA5 (inputs with pull-ups). */

void input_init(void);

/* Every 5 ms (vTaskDelayUntil): decodes the encoder, steps the display mode
 * (clockwise = next, counterclockwise = previous) and overwrites the new mode
 * into xModeQueue; logs mode changes and button presses. */
void InputTask(void *pvParameters);
