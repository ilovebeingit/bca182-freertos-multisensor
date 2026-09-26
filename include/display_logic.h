#pragma once

/* SSD1306 frame buffer, 5x7 font engine and screen layout.
 * Hardware-independent (no HAL/FreeRTOS).
 *
 * Frame buffer layout (the SSD1306 GDDRAM format): 128 columns x 8 pages,
 * one byte per (column, page). A page is a horizontal strip 8 pixels tall;
 * bit 0 of a byte is the top pixel of its strip, bit 7 the bottom. So pixel
 * (x, y) is bit (y % 8) of byte fb[(y / 8) * 128 + x]. */

#include <stdint.h>
#include "system_state.h"

constexpr uint16_t kDisplayWidth = 128;
constexpr uint16_t kDisplayHeight = 64;
constexpr uint16_t kDisplayPages = kDisplayHeight / 8;
constexpr uint16_t kDisplayBufferSize = kDisplayWidth * kDisplayPages;   /* 1024 */

/* Font cell at scale 1: 5x7 glyph plus 1 column / 1 row of spacing. */
constexpr uint8_t kGlyphWidth = 5;
constexpr uint8_t kGlyphHeight = 7;
constexpr uint8_t kCharAdvance = kGlyphWidth + 1;
constexpr uint8_t kLineHeight = kGlyphHeight + 1;

enum class DisplayMode : uint8_t {
    TEMPERATURE,
    HUMIDITY,
    LIGHT,
    MOTION,
};

/* Mode shown at boot; InputTask and DisplayTask both start from it. */
constexpr DisplayMode kInitialDisplayMode = DisplayMode::TEMPERATURE;

/* Encoder navigation. Clockwise: TEMPERATURE -> HUMIDITY -> LIGHT -> MOTION
 * -> TEMPERATURE; counterclockwise is the reverse. Both wrap around. */
DisplayMode nextDisplayMode(DisplayMode mode);
DisplayMode previousDisplayMode(DisplayMode mode);

void display_clear(uint8_t *fb);
void display_set_pixel(uint8_t *fb, int16_t x, int16_t y);

/* Draws printable ASCII 32..126 (anything else draws as '?') with its top-left
 * corner at (x, y); every font pixel becomes a scale x scale block. Pixels
 * outside the screen are clipped. */
void display_draw_char(uint8_t *fb, int16_t x, int16_t y, char c, uint8_t scale);
void display_draw_string(uint8_t *fb, int16_t x, int16_t y, const char *str, uint8_t scale);

/* Width in pixels of str at scale (no trailing spacing column). */
uint16_t display_text_width(const char *str, uint8_t scale);

/* Largest scale in 1..max_scale at which str fits the screen width. */
uint8_t display_fit_scale(const char *str, uint8_t max_scale);

const char *display_mode_label(DisplayMode mode);

/* The value text for one mode, e.g. "23.5 C", "41.2 %", "57 %", "DETECTED". */
void display_format_value(char *buf, uint16_t size, DisplayMode mode,
                          const SensorData *sample, bool motion_detected);

/* Renders the whole screen for one mode:
 *   ROOM MONITOR
 *   (blank line)
 *   <label for mode>
 *   <value, larger scale>
 * Only the measurement for `mode` is drawn (FR-05: one at a time). */
void display_render_screen(uint8_t *fb, DisplayMode mode,
                           const SensorData *sample, bool motion_detected);
