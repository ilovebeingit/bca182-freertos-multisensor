#pragma once

/* Buzzer decisions and timing. Hardware-independent (no HAL/FreeRTOS). */

#include <stdint.h>

/* One alarm burst: kAlarmBurstCycles on/off cycles of kAlarmHalfPeriodMs each
 * way, then kAlarmBurstPauseMs of silence. With no motion the alarm task
 * re-checks every kAlarmIdlePollMs. */
constexpr uint32_t kAlarmBurstCycles = 20;
constexpr uint32_t kAlarmHalfPeriodMs = 2;
constexpr uint32_t kAlarmBurstPauseMs = 100;
constexpr uint32_t kAlarmIdlePollMs = 200;

bool alarm_should_sound(uint8_t motion_flag);
