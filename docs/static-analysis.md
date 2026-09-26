# Static analysis

`pio check` (cppcheck) results for this firmware: every finding, its cause,
and what was done about it. Nothing has been left out.

- **Tool:** cppcheck 2.11 (`platformio/tool-cppcheck @ 1.21100.230717`), run by
  PlatformIO Core, version 6.2.0. PlatformIO's default checks: `warning, style, performance,
  portability, unusedFunction`.
- **Commands:** `pio check -e bluepill_f103c8` and `pio check -e native`.
- **Date:** 2026-09-26.

## Summary

| | High | Medium | Low | Total |
|---|---|---|---|---|
| First run, `bluepill_f103c8` | 0 | 0 | 43 | 43 |
| First run, `native` | 0 | 0 | 29 | 29 |
| **First run, total** | **0** | **0** | **72** | **72** |
| Final run, `bluepill_f103c8` | 0 | 0 | 12 | 12 |
| Final run, `native` | 0 | 0 | 0 | 0 |

How the 72 findings were resolved:
- **2 fixed.** Our own C-style casts in `log_line()`.
- **58 false positives** (`unusedFunction`, 29 in each environment). See below.
- **12 accepted, not fixed.** C-style casts inside CMSIS and FreeRTOS
  macros, not in project code. They remain in the final report.

There were no high- or medium-severity findings, and none pointed to a
functional bug.

## Why the 58 `unusedFunction` findings are false positives

`pio check` runs cppcheck once per source file (17 separate runs, confirmed
with `pio check -v`). cppcheck's `unusedFunction` check only works when it
sees the whole program at once. In a per-file run, every function called only
from another file looks unused: for example `alarm_init` (called from
`main.cpp`) and every task function (passed to `xTaskCreate` in
`main.cpp`).

The fix was not to lose the check but to run it where it works:
1. In `platformio.ini`, both environments set
   `check_flags = cppcheck: --suppress=unusedFunction` for the per-file runs,
   with a comment explaining why.
2. The check is run once over the whole program instead:

   ```
   cppcheck --enable=unusedFunction --language=c++ --quiet -Iinclude -DSTM32F103xB -DUSE_HAL_DRIVER src/*.cpp
   ```

   Result: **no unused functions.** To confirm the whole-program run can
   actually detect one, a copy of the sources with a deliberately unused
   function added was checked. That function was reported, and nothing else.

## About the 12 accepted C-style casts

Each one is at a line where our code uses a vendor macro that is itself
defined with a C-style cast:
- CMSIS: `DWT`, `CoreDebug`, `SCB` and `I2C1`, e.g.
  `#define DWT ((DWT_Type *) DWT_BASE)`.
- FreeRTOS: `xSemaphoreGive(x)`, which expands to
  `xQueueGenericSend((QueueHandle_t)(x), ...)`.

The only ways to remove them would be to edit the vendor headers, or to
duplicate their register definitions with our own casts. Both are worse than
the finding, so they are accepted.

**Inline suppressions did not work here.** Suppressing them one line at a
time (`// cppcheck-suppress cstyleCast`) was tried. PlatformIO does pass
`--inline-suppr`, but `pio check` still reported all 12. Running the same
cppcheck 2.11 directly, with the same flags, the real CMSIS/HAL/FreeRTOS
include paths and both relative and absolute file paths, honoured the
suppressions every time, so the cause inside PlatformIO's invocation could
not be isolated. The suppression comments were removed again rather than
left in as ineffective noise, and the findings stay visible in the report.

## All findings

Line numbers are from the first run. Later edits moved some of the 12
accepted findings without changing them:
- Fixing `serial_log.cpp:45-46` moved finding 39 to line 53.
- Adding `app_main()` moved finding 29 to `main.cpp:36`. Replacing the
  FreeRTOS port (which removed the application's `SysTick_Handler`) then
  moved it to `main.cpp:30`.
- A longer comment above the DHT22 read moved findings 12-13 from
  `dht22.cpp:99` and `:104` to `:104` and `:109`.

The latest run (after the port replacement, which added `include/portmacro.h`
and the new `port.c`) still reports exactly these 12 accepted findings and
nothing new.

| # | Env | Severity | Finding | File/Line | Cause | Resolution |
|---|---|---|---|---|---|---|
| 1 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/alarm.cpp:32` | `alarm_init` is called from main.cpp:55, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 2 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/alarm.cpp:84` | Task function `AlarmTask` is passed to `xTaskCreate` in main.cpp:69; the per-file run sees neither that nor the scheduler's call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 3 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/alarm_logic.cpp:3` | `evaluateTemperature` is called from alarm.cpp:94, display.cpp:164, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 4 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/alarm_logic.cpp:13` | `alarmStateName` is called from alarm.cpp:97, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 5 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/dht22.cpp:36` | Cast inside the CMSIS `DWT` / `CoreDebug` macros (`((DWT_Type *) DWT_BASE)`), not in our code | **Accepted, not fixed.** Vendor (CMSIS) macro; fixing it means editing or duplicating the device headers |
| 6 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/dht22.cpp:42` | Cast inside the CMSIS `DWT` / `CoreDebug` macros (`((DWT_Type *) DWT_BASE)`), not in our code | **Accepted, not fixed.** Vendor (CMSIS) macro; fixing it means editing or duplicating the device headers |
| 7 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/dht22.cpp:53` | Cast inside the CMSIS `DWT` / `CoreDebug` macros (`((DWT_Type *) DWT_BASE)`), not in our code | **Accepted, not fixed.** Vendor (CMSIS) macro; fixing it means editing or duplicating the device headers |
| 8 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/dht22.cpp:54` | Cast inside the CMSIS `DWT` / `CoreDebug` macros (`((DWT_Type *) DWT_BASE)`), not in our code | **Accepted, not fixed.** Vendor (CMSIS) macro; fixing it means editing or duplicating the device headers |
| 9 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/dht22.cpp:55` | Cast inside the CMSIS `DWT` / `CoreDebug` macros (`((DWT_Type *) DWT_BASE)`), not in our code | **Accepted, not fixed.** Vendor (CMSIS) macro; fixing it means editing or duplicating the device headers |
| 10 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/dht22.cpp:59` | Cast inside the CMSIS `DWT` / `CoreDebug` macros (`((DWT_Type *) DWT_BASE)`), not in our code | **Accepted, not fixed.** Vendor (CMSIS) macro; fixing it means editing or duplicating the device headers |
| 11 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/dht22.cpp:61` | Cast inside the CMSIS `DWT` / `CoreDebug` macros (`((DWT_Type *) DWT_BASE)`), not in our code | **Accepted, not fixed.** Vendor (CMSIS) macro; fixing it means editing or duplicating the device headers |
| 12 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/dht22.cpp:99` | Cast inside the CMSIS `DWT` / `CoreDebug` macros (`((DWT_Type *) DWT_BASE)`), not in our code | **Accepted, not fixed.** Vendor (CMSIS) macro; fixing it means editing or duplicating the device headers |
| 13 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/dht22.cpp:104` | Cast inside the CMSIS `DWT` / `CoreDebug` macros (`((DWT_Type *) DWT_BASE)`), not in our code | **Accepted, not fixed.** Vendor (CMSIS) macro; fixing it means editing or duplicating the device headers |
| 14 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/dht22.cpp:49` | `dht22_init` is called from sensors.cpp:50, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 15 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/dht22.cpp:64` | `dht22_read` is called from sensors.cpp:98, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 16 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/dht22_logic.cpp:9` | `dht22_decode` is called from dht22.cpp:113, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 17 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/dht22_logic.cpp:36` | `format_tenths_2dp` is called from sensors.cpp:119, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 18 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/dht22_logic.cpp:44` | `dht22_status_name` is called from sensors.cpp:101, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 19 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/display.cpp:92` | Cast inside the CMSIS `I2C1` macro (`((I2C_TypeDef *) I2C1_BASE)`), not in our code | **Accepted, not fixed.** Vendor (CMSIS) macro; fixing it means editing or duplicating the device headers |
| 20 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/display.cpp:82` | `display_init` is called from main.cpp:57, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 21 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/display.cpp:104` | Task function `DisplayTask` is passed to `xTaskCreate` in main.cpp:66; the per-file run sees neither that nor the scheduler's call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 22 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/display_logic.cpp:164` | `nextDisplayMode` is called from input.cpp:51, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 23 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/display_logic.cpp:174` | `previousDisplayMode` is called from input.cpp:51, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 24 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/display_logic.cpp:231` | `display_render_screen` is called from display.cpp:167, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 25 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/input.cpp:15` | `input_init` is called from main.cpp:56, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 26 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/input.cpp:27` | Task function `InputTask` is passed to `xTaskCreate` in main.cpp:67; the per-file run sees neither that nor the scheduler's call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 27 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/input_logic.cpp:3` | `encoder_step` is called from input.cpp:46, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 28 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/input_logic.cpp:10` | `button_is_pressed` is called from input.cpp:59, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 29 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/main.cpp:34` | Cast inside the CMSIS `SCB` macro (`((SCB_Type *) SCB_BASE)`), not in our code | **Accepted, not fixed.** Vendor (CMSIS) macro; fixing it means editing or duplicating the device headers. Now reported at line 36 after `app_main()` was added |
| 30 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/motion.cpp:12` | `motion_init` is called from main.cpp:54, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 31 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/motion.cpp:24` | Task function `MotionTask` is passed to `xTaskCreate` in main.cpp:68; the per-file run sees neither that nor the scheduler's call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 32 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/motion_logic.cpp:3` | `pir_motion_detected` is called from motion.cpp:35, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 33 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/rtos_objects.cpp:13` | `rtos_objects_create` is called from main.cpp:59, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 34 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/sensors.cpp:16` | `sensors_init` is called from main.cpp:53, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 35 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/sensors.cpp:72` | Task function `SensorTask` is passed to `xTaskCreate` in main.cpp:65; the per-file run sees neither that nor the scheduler's call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 36 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/sensors_logic.cpp:3` | `lightPercentFromAdc` is called from sensors.cpp:111, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 37 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/serial_log.cpp:45` | Our own C-style cast `(const uint8_t *)` of the text passed to `HAL_UART_Transmit` | **Fixed:** `reinterpret_cast<const uint8_t *>` (and `static_cast` for the length) |
| 38 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/serial_log.cpp:46` | Our own C-style cast `(const uint8_t *)` of the text passed to `HAL_UART_Transmit` | **Fixed:** `reinterpret_cast<const uint8_t *>` (and `static_cast` for the length) |
| 39 | bluepill_f103c8 | low (style) | `cstyleCast` | `src/serial_log.cpp:49` | Cast inside FreeRTOS's `xSemaphoreGive()` macro: `( QueueHandle_t ) ( xSemaphore )` | **Accepted, not fixed.** Vendor macro; avoiding it would mean bypassing the documented FreeRTOS API. Now reported at line 53 because the fix above moved it |
| 40 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/serial_log.cpp:15` | `serial_log_init` is called from main.cpp:49, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 41 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/serial_log.cpp:38` | `log_line` is called from alarm.cpp:71, display.cpp:112, input.cpp:34, main.cpp:50, motion.cpp:27, sensors.cpp:78, system_state.cpp:18, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 42 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/system_state.cpp:13` | Task function `StateTask` is passed to `xTaskCreate` in main.cpp:70; the per-file run sees neither that nor the scheduler's call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 43 | bluepill_f103c8 | low (style) | `unusedFunction` | `src/system_state_logic.cpp:3` | `evaluateSystemState` is called from system_state.cpp:40, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 44 | native | low (style) | `unusedFunction` | `src/alarm.cpp:32` | `alarm_init` is called from main.cpp:55, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 45 | native | low (style) | `unusedFunction` | `src/alarm.cpp:84` | Task function `AlarmTask` is passed to `xTaskCreate` in main.cpp:69; the per-file run sees neither that nor the scheduler's call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 46 | native | low (style) | `unusedFunction` | `src/alarm_logic.cpp:3` | `evaluateTemperature` is called from alarm.cpp:94, display.cpp:164, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 47 | native | low (style) | `unusedFunction` | `src/alarm_logic.cpp:13` | `alarmStateName` is called from alarm.cpp:97, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 48 | native | low (style) | `unusedFunction` | `src/dht22.cpp:49` | `dht22_init` is called from sensors.cpp:50, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 49 | native | low (style) | `unusedFunction` | `src/dht22.cpp:64` | `dht22_read` is called from sensors.cpp:98, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 50 | native | low (style) | `unusedFunction` | `src/dht22_logic.cpp:9` | `dht22_decode` is called from dht22.cpp:113, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 51 | native | low (style) | `unusedFunction` | `src/dht22_logic.cpp:36` | `format_tenths_2dp` is called from sensors.cpp:119, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 52 | native | low (style) | `unusedFunction` | `src/dht22_logic.cpp:44` | `dht22_status_name` is called from sensors.cpp:101, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 53 | native | low (style) | `unusedFunction` | `src/display.cpp:82` | `display_init` is called from main.cpp:57, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 54 | native | low (style) | `unusedFunction` | `src/display.cpp:104` | Task function `DisplayTask` is passed to `xTaskCreate` in main.cpp:66; the per-file run sees neither that nor the scheduler's call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 55 | native | low (style) | `unusedFunction` | `src/display_logic.cpp:164` | `nextDisplayMode` is called from input.cpp:51, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 56 | native | low (style) | `unusedFunction` | `src/display_logic.cpp:174` | `previousDisplayMode` is called from input.cpp:51, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 57 | native | low (style) | `unusedFunction` | `src/display_logic.cpp:231` | `display_render_screen` is called from display.cpp:167, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 58 | native | low (style) | `unusedFunction` | `src/input.cpp:15` | `input_init` is called from main.cpp:56, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 59 | native | low (style) | `unusedFunction` | `src/input.cpp:27` | Task function `InputTask` is passed to `xTaskCreate` in main.cpp:67; the per-file run sees neither that nor the scheduler's call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 60 | native | low (style) | `unusedFunction` | `src/input_logic.cpp:3` | `encoder_step` is called from input.cpp:46, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 61 | native | low (style) | `unusedFunction` | `src/input_logic.cpp:10` | `button_is_pressed` is called from input.cpp:59, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 62 | native | low (style) | `unusedFunction` | `src/motion.cpp:12` | `motion_init` is called from main.cpp:54, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 63 | native | low (style) | `unusedFunction` | `src/motion.cpp:24` | Task function `MotionTask` is passed to `xTaskCreate` in main.cpp:68; the per-file run sees neither that nor the scheduler's call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 64 | native | low (style) | `unusedFunction` | `src/motion_logic.cpp:3` | `pir_motion_detected` is called from motion.cpp:35, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 65 | native | low (style) | `unusedFunction` | `src/rtos_objects.cpp:13` | `rtos_objects_create` is called from main.cpp:59, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 66 | native | low (style) | `unusedFunction` | `src/sensors.cpp:16` | `sensors_init` is called from main.cpp:53, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 67 | native | low (style) | `unusedFunction` | `src/sensors.cpp:72` | Task function `SensorTask` is passed to `xTaskCreate` in main.cpp:65; the per-file run sees neither that nor the scheduler's call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 68 | native | low (style) | `unusedFunction` | `src/sensors_logic.cpp:3` | `lightPercentFromAdc` is called from sensors.cpp:111, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 69 | native | low (style) | `unusedFunction` | `src/serial_log.cpp:15` | `serial_log_init` is called from main.cpp:49, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 70 | native | low (style) | `unusedFunction` | `src/serial_log.cpp:38` | `log_line` is called from alarm.cpp:71, display.cpp:112, input.cpp:34, main.cpp:50, motion.cpp:27, sensors.cpp:78, system_state.cpp:18, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 71 | native | low (style) | `unusedFunction` | `src/system_state.cpp:13` | Task function `StateTask` is passed to `xTaskCreate` in main.cpp:70; the per-file run sees neither that nor the scheduler's call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
| 72 | native | low (style) | `unusedFunction` | `src/system_state_logic.cpp:3` | `evaluateSystemState` is called from system_state.cpp:40, but `pio check` runs cppcheck on one file at a time, so it never sees the call | False positive. `unusedFunction` disabled for per-file runs (`check_flags`); the whole-program run reports 0 unused functions |
