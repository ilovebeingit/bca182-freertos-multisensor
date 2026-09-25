#pragma once

/* SSD1306 framebuffer drawing and dashboard layout. Hardware-independent
 * (no HAL/FreeRTOS): works on a caller-owned 128x64 1-bpp buffer laid out as
 * 8 pages of 128 column bytes, the SSD1306 horizontal-addressing format. */

#include <stdint.h>
#include "system_state.h"

constexpr uint16_t kDisplayWidth = 128;
constexpr uint16_t kDisplayPages = 8;
constexpr uint16_t kDisplayBufferSize = kDisplayWidth * kDisplayPages;

void display_clear(uint8_t *fb);

/* 5x7 font, ' ' to 'Z'; anything else draws as a space. y is rounded down to
 * a page (multiple of 8). */
void display_draw_char(uint8_t *fb, uint8_t x, uint8_t y, char c);
void display_write_string(uint8_t *fb, uint8_t x, uint8_t y, const char *str);

/* Clears fb and draws the dashboard: light from the sensor sample, plus the
 * current motion state and encoder count. */
void display_render_dashboard(uint8_t *fb, const SensorData_t *sample,
                              bool motion_detected, int32_t encoder_count);
