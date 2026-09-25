#pragma once

/* KY-040 rotary encoder: CLK PA3, DT PA4, SW PA5 (inputs with pull-ups). */

void input_init(void);

/* Polls the encoder every 10 ms, updating g_encoder_count and g_button_flag. */
void InputTask(void *pvParameters);
