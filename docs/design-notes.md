# Design notes: tasks and RTOS objects

How the firmware's tasks communicate, and why each queue, mutex, queue set
and event-group bit exists. All kernel objects are created in
`rtos_objects_create()` (`src/rtos_objects.cpp`) before the scheduler starts.

Runtime behaviour has not been verified in Wokwi: its Cortex-M emulation
cannot run the FreeRTOS scheduler (see `docs/dev-log.md`). Everything below
describes the design as implemented.

## Tasks

| Task | Trigger | Prio | Stack (words) | Why this priority |
|---|---|---|---|---|
| InputTask | 5 ms, `vTaskDelayUntil` | 3 | 128 | Encoder edges are only a few ms apart; a late sample loses steps. |
| MotionTask | 100 ms, `vTaskDelayUntil` | 3 | 128 | PIR state drives waking up (FR-10); it must not queue behind slower work. |
| SensorTask | 2000 ms, `vTaskDelayUntil` | 2 | 256 | Periodic sensing; the DHT22's microsecond timing is protected by a critical section, not by priority. |
| AlarmTask | each sample on `xAlarmQueue` (500 ms timeout) | 2 | 128 | Buzzer response within one sample period is enough. |
| StateTask | `EVENT_MOTION`, 15 s timeout | 2 | 128 | Works on a 15 s timescale; must not delay input or motion sampling. |
| DisplayTask | queue set, 250 ms timeout | 1 | 256 | A full OLED flush takes about 100 ms of I2C; lowest priority so it never delays sampling, and a human does not notice 100 ms. |

Every task blocks in each loop iteration; none busy-waits.

## Queues

All three queues hold one item and are written with `xQueueOverwrite()`: a
consumer only ever needs the newest value, and a producer never blocks.

### `xDisplayQueue` (1 x `SensorData`)
- **Producer:** SensorTask, every 2 s while ACTIVE, and only after a successful
  DHT22 read (a failed read skips the cycle).
- **Consumer:** DisplayTask, through the `xDisplayEvents` queue set.
- **Why:** carries the measurements to the only task that owns the OLED.

### `xAlarmQueue` (1 x `SensorData`)
- **Producer:** SensorTask, the same sample as `xDisplayQueue`.
- **Consumer:** AlarmTask (`xQueueReceive`, 500 ms timeout).
- **Why a second queue:** a queue item is removed by the task that receives it.
  With one queue shared by DisplayTask and AlarmTask, whichever received first
  would take the sample, and the other would miss it (CLAUDE.md: one queue per
  consumer).

### `xModeQueue` (1 x `DisplayMode`)
- **Producer:** InputTask, when the encoder steps while ACTIVE.
- **Consumer:** DisplayTask, through `xDisplayEvents`.
- **Why:** the encoder is decoded in a fast task and the screen is drawn in a
  slow one. The queue hands over the chosen mode without a shared variable,
  and wakes DisplayTask immediately so the screen changes at once.

## Queue set

### `xDisplayEvents` = { `xDisplayQueue`, `xModeQueue` }, length 2
- **Consumer:** DisplayTask (`xQueueSelectFromSet`, 250 ms timeout).
- **Why:** DisplayTask must wake for whichever comes first, a new sample or a
  new mode. A task can block on only one object; a queue set lets it block on
  both. The length is the sum of its members' lengths.
- **Why the timeout:** event groups cannot be added to a queue set, so
  DisplayTask wakes at least every 250 ms to check `EVENT_ACTIVE`,
  `EVENT_MOTION` and `EVENT_ALARM`. It redraws only if something has changed.

## Mutex

### `serialMutex`
- **Users:** every task, through `log_line()` (`src/serial_log.cpp`).
- **Resource:** USART1 and its HAL handle (`huart1`).
- **Why:** `HAL_UART_Transmit()` is not re-entrant. Its busy check is a
  non-atomic read-then-set, so two tasks transmitting at once can drop lines
  (`HAL_BUSY`), interleave bytes or corrupt the driver state. Holding the mutex
  for a whole line (text plus `\r\n`) prevents this. A mutex rather than a
  binary semaphore gives priority inheritance: a priority-2 task holding it is
  raised while a priority-3 task waits, so the wait is bounded.
- Before the scheduler starts, only `main()` runs, so `log_line()` skips the
  mutex (it may not exist yet).

## Event group `xSystemEvents`

System-wide status flags. Several tasks read each bit, and each bit has
exactly one writer.

| Bit | Meaning | Set by | Cleared by | Read by |
|---|---|---|---|---|
| `EVENT_ACTIVE` (BIT0) | system is ACTIVE (FR-08) | `rtos_objects_create()` at boot; StateTask on INACTIVE -> ACTIVE | StateTask on ACTIVE -> INACTIVE | DisplayTask (blank when clear), InputTask (encoder only when set), SensorTask (sense only when set), AlarmTask (buzzer only when set) |
| `EVENT_MOTION` (BIT1) | PIR currently reports motion (FR-04) | MotionTask, every 100 ms while the PIR is high | MotionTask, every 100 ms while the PIR is low | StateTask (waits on it, FR-09/FR-10), DisplayTask (MOTION screen), SensorTask (`SensorData.motionDetected`) |
| `EVENT_ALARM` (BIT2) | buzzer is sounding (FR-07) | AlarmTask when the buzzer starts | AlarmTask when it stops | DisplayTask (`ALARM LOW` / `ALARM HIGH` line) |

- **Why an event group:** these are yes/no conditions that several tasks need
  to *read* at any time. A queue would hand each item to one reader only. The
  event group also lets StateTask *block* until motion appears, with the 15 s
  inactivity timeout as the wait's timeout.
- `EVENT_MOTION` is a level bit, so StateTask does not clear it on exit (other
  readers need its current value). Instead, after seeing motion it blocks for
  500 ms before waiting again, so it cannot spin (see `docs/dev-log.md`).

## Other shared state

- There are no global variables for inter-task data.
- Hardware handles are `static` in the module that owns them, and each is used
  by one task only: `hadc1` (SensorTask), `hi2c1` and the frame buffer
  (DisplayTask), `htim1` (AlarmTask). `huart1` is shared, behind `serialMutex`.
- The DHT22 bit capture (about 4-5 ms) runs inside
  `taskENTER_CRITICAL()`/`taskEXIT_CRITICAL()`, so no context switch or
  kernel-level interrupt can stretch a pulse measurement. Decoding and logging
  happen after the critical section ends.
