#pragma once

/* Rotary encoder / button decisions. Hardware-independent (no HAL/FreeRTOS). */

#include <stdint.h>

/* Quadrature step from the encoder CLK/DT levels (true = high). A step is
 * counted on a CLK falling edge: +1 when DT differs from CLK, -1 otherwise.
 * Returns 0 when there is no falling edge. */
int8_t encoder_step(bool prev_clk_high, bool clk_high, bool dt_high);

/* The encoder switch is active low (input has a pull-up). */
bool button_is_pressed(bool sw_high);
