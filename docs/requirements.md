# Requirements: BCA182 Lab 1, FreeRTOS Multisensor Room Monitor

The requirements this project is built and tested against. Functional tests
FT-01..FT-10 in [`test-plan.md`](test-plan.md) correspond one-to-one to
FR-01..FR-10 below; the design decisions that meet them are in
[`design-notes.md`](design-notes.md).

## Platform and design constraints

- PlatformIO with `platform = ststm32`, `board = bluepill_f103c8`,
  `framework = stm32cube`. No Arduino framework or Arduino APIs.
- STM32 HAL for peripherals; native FreeRTOS APIs for tasks, queues, mutex,
  event group and timing.
- C++ (`src/*.cpp`). `app_main()` is the application entry, called from
  `main()`. `main.cpp` stays short: hardware init, then create the RTOS
  objects, then create the tasks, then start the scheduler.
- No uncontrolled busy loops. Every task blocks (`vTaskDelay`,
  `vTaskDelayUntil`, a queue wait or a notification wait).
- At least one periodic task uses `vTaskDelayUntil()` (SensorTask, 2000 ms).
- No unsynchronized globals for inter-task data: use queues, an event group or
  task notifications. Shared resources need a mutex.
- Every queue, mutex and event group must have a real role in the design, and
  each is documented in `docs/design-notes.md` (resource, producer, consumer,
  why it exists).
- Hardware-independent decision logic lives in files that include no HAL or
  FreeRTOS headers, so it can be unit tested with `platform = native`.

## Hardware (Wokwi)

| Pin | Device |
|---|---|
| PA0 | DHT22 data (bit-banged, needs microsecond timing; the read is protected by a critical section) |
| PA1 | LDR module AO (ADC1_IN1), reported as 0-100 % relative light, not lux |
| PA2 | PIR motion OUT (digital input) |
| PA3 / PA4 / PA5 | KY-040 rotary encoder CLK / DT / SW (inputs with pull-up) |
| PB6 / PB7 | SSD1306 128x64 OLED, I2C1 SCL / SDA, address 0x3C |
| PA8 | Buzzer (hardware PWM preferred: TIM1_CH1, about 1 kHz) |
| PC13 | Heartbeat LED |
| PA9 / PA10 | USART1 serial log (Wokwi Serial Monitor) |

## Functional requirements

| ID | Requirement |
|---|---|
| FR-01 | Periodic temperature measurement |
| FR-02 | Periodic humidity measurement |
| FR-03 | Ambient light level |
| FR-04 | PIR motion detection |
| FR-05 | The OLED shows ONE measurement at a time |
| FR-06 | The encoder cycles Temperature → Humidity → Light → Motion clockwise, and the reverse counterclockwise, with wraparound |
| FR-07 | The buzzer sounds when the TEMPERATURE is outside 18.0-30.0 °C (the alarm is temperature-based, not motion-based) |
| FR-08 | The system has ACTIVE and INACTIVE states |
| FR-09 | No motion for 15 s → INACTIVE |
| FR-10 | Motion → ACTIVE |

**States (FR-08):**
- **ACTIVE:** OLED on, sensing, encoder active, alarm active.
- **INACTIVE:** OLED blank, reduced display work, PIR still monitored.

## Tasks

Starting priorities; the final priorities must be justified by scheduling
urgency and latency.

| Task | Job | Trigger | Priority | IPC |
|---|---|---|---|---|
| MotionTask | PIR | 100 ms periodic | 3 | event group |
| InputTask | encoder | 5 ms periodic | 3 | queue or notification → DisplayTask |
| SensorTask | DHT22 + LDR | 2 s, `vTaskDelayUntil` | 2 | queue(s) |
| AlarmTask | evaluate temperature, drive buzzer | sensor update | 2 | queue |
| StateTask | ACTIVE/INACTIVE state machine | event group + 15 s timeout | 2 | event group |
| DisplayTask | owns the OLED | data or mode update | 1 | queue |

## Data types

```cpp
struct SensorData { float temperature; float humidity; int lightLevel; bool motionDetected; };
enum class DisplayMode { TEMPERATURE, HUMIDITY, LIGHT, MOTION };
enum class AlarmState { NORMAL, LOW_TEMPERATURE, HIGH_TEMPERATURE };
```

## Event bits

| Bit | Name |
|---|---|
| BIT0 | `EVENT_ACTIVE` |
| BIT1 | `EVENT_MOTION` |
| BIT2 | `EVENT_ALARM` |

The producer, consumers, and when each bit is set and cleared are documented
in `docs/design-notes.md`.

## Inter-task communication rules

- A queue has one consumer per item. If both DisplayTask and AlarmTask need
  `SensorData`, each gets its own queue.
- A mutex protects the shared serial (USART) output.

## Serial output

The boot banner is `BCA182 FreeRTOS Multisensor`, then `System starting...`.

## Unit tests

Unity tests run with `pio test -e native`: at least 13 tests, aiming for 15
or more, exercising real decision logic.

| Function | Cases |
|---|---|
| `evaluateTemperature(float)` | 5: below low, exactly low, normal, exactly high, above high |
| `nextDisplayMode` / `previousDisplayMode` | 4, including wraparound |
| `evaluateSystemState(...)` | 4: ACTIVE without timeout, ACTIVE with timeout, INACTIVE without motion, INACTIVE with motion |

## Repository contents

- `include/` and `src/` each with `sensors`, `display`, `input`, `alarm`,
  `motion`, `system_state` and `rtos_objects` (plus `main.cpp`). These may
  differ only if justified.
- Also `test/`, `docs/`, `README.md`, `diagram.json`, `wokwi.toml` and
  `platformio.ini`. No build artifacts committed.

## Deliverables

- `README.md` in public portfolio style, with the sections and five visuals
  required by the assignment.
- `docs/laboratory-report.pdf`.
- A Hackster.io article.
- The instructor added as a collaborator on the GitHub repository.
