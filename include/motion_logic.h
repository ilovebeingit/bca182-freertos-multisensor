#pragma once

/* PIR decision. Hardware-independent (no HAL/FreeRTOS). */

/* The PIR output is active high. */
bool pir_motion_detected(bool out_high);
