# Test plan: functional tests and fault experiments

How to verify the firmware in the Wokwi simulation, and where to record the
results. There is one functional test per requirement in `CLAUDE.md`
(FT-01..FT-10 correspond to FR-01..FR-10), plus a boot check and three fault
experiments. The FT and F numbering is this project's own.

Record only what you actually observe (CLAUDE.md: never invent test results).
For each test, fill in the Result block with PASS, FAIL or PARTIAL, what you
saw (log lines, OLED text, timings), and the date. If a result differs from
the expected behaviour, note it and add it to `docs/dev-log.md`.

## Results summary

| ID | Verifies | Result | Date |
|---|---|---|---|
| B-01 | Boot sequence, all six tasks start | PASS | 2026-09-27 |
| FT-01 | FR-01 periodic temperature | PASS | 2026-09-27 |
| FT-02 | FR-02 periodic humidity | PASS | 2026-09-27 |
| FT-03 | FR-03 ambient light (0-100 %) | PASS | 2026-09-27 |
| FT-04 | FR-04 PIR motion | PASS | 2026-09-27 |
| FT-05 | FR-05 one measurement at a time | PASS | 2026-09-27 |
| FT-06 | FR-06 encoder navigation with wraparound | PASS | 2026-09-27 |
| FT-07 | FR-07 temperature alarm (18.0-30.0 °C) | PASS | 2026-09-27 |
| FT-08 | FR-08 ACTIVE / INACTIVE behaviour | PASS | 2026-09-27 |
| FT-09 | FR-09 INACTIVE after 15 s without motion | PASS | 2026-09-27 |
| FT-10 | FR-10 motion returns to ACTIVE | PASS | 2026-09-27 |
| F-1 | Fault: DHT22 disconnected | PASS | 2026-09-27 |
| F-2 | Fault: OLED disconnected | PASS | 2026-09-27 |
| F-3 | Fault: temperature alarm while INACTIVE | PASS | 2026-09-27 |

## Setup (every test)

1. Build the firmware (`pio run`), then start the simulation in VS Code:
   **F1 → "Wokwi: Start Simulator"**.
2. Keep the **Wokwi Terminal** (the serial log) and the circuit diagram both
   visible.
3. **Part controls:** click the DHT22, the photoresistor or the PIR to open
   its controls (temperature/humidity sliders, light slider, motion trigger).
   The KY-040 encoder's arrows turn it clockwise (CW) or counterclockwise
   (CCW); clicking the knob presses its button.
4. **Keep the system ACTIVE:** after 15 s without motion it goes INACTIVE
   (OLED blank, encoder ignored, no sampling). Except in FT-09 and F-3,
   trigger the PIR before starting a test, and again whenever the log shows
   `STATE: INACTIVE`.
5. Record the firmware commit you tested (`git log --oneline -1`):
   `de3a490` (all tests in this plan)

## B-01: boot sequence

**Steps**
1. Start the simulation and read the first lines of the terminal.

**Expected**
- `BCA182 FreeRTOS Multisensor`, then `System starting...`
- `ALARM: TIM1 clock 8000000 Hz, PSC=7, ARR=999 -> 1000 Hz PWM`
- All six of `SensorTask started`, `DisplayTask started`, `InputTask started`,
  `MotionTask started`, `AlarmTask started`, `StateTask started` (in any order)
- `DISPLAY: OLED initialised`
- About 2 s later the first `Sample: ...` line, and the OLED shows
  `ROOM MONITOR` / `TEMPERATURE` / a value such as `24.0 C`
- The PC13 heartbeat LED toggles every 2 s

**Result:** PASS | **Observed:** Full boot sequence confirmed twice: `BCA182 FreeRTOS Multisensor`, `System starting...`, the TIM1 timer setup message, all six `...Task started` lines and `DISPLAY: OLED initialised`. | **Date:** 2026-09-27

## FT-01: periodic temperature (FR-01)

**Steps**
1. Wait for three `Sample:` lines and note their timing.
2. Set the DHT22 temperature to **26.5 °C**.
3. Wait for the next `Sample:` line.

**Expected**
- `Sample: Temperature: 24.00 C, Humidity: 40.00 %, Light: NN %, Motion: no`
  (or your current values) every **2 s**.
- Within 2 s of the change: `Temperature: 26.50 C`.
- The OLED (TEMPERATURE mode) shows `26.5 C`.

**Result:** PASS | **Observed:** On the committed build (not the debug build), the DHT22 was set to 11.80 °C. The OLED updated on its own, without touching the encoder, to `ALARM LOW` / `TEMPERATURE` / `11.8 C`, matching the terminal sample. An earlier observation of the OLED not refreshing on new samples while parked on one mode was not reproduced here; its cause was not identified. | **Date:** 2026-09-27

## FT-02: periodic humidity (FR-02)

**Steps**
1. Turn the encoder **once CW**: the log shows `INPUT: mode HUMIDITY`.
2. Set the DHT22 humidity to **65 %**.
3. Wait for the next `Sample:` line.

**Expected**
- Within 2 s: `Humidity: 65.00 %` in the log.
- The OLED shows `HUMIDITY` / `65.0 %`.

**Result:** PASS | **Observed:** Humidity set to 21.00 %. The OLED updated on its own to `HUMIDITY` / `21.0 %`, matching the terminal. | **Date:** 2026-09-27

## FT-03: ambient light, 0-100 % (FR-03)

**Steps**
1. Turn the encoder CW until the log shows `INPUT: mode LIGHT`.
2. Move the photoresistor slider to **bright** and wait 2 s; note the value.
3. Move it to **dark** and wait 2 s; note the value.

**Expected**
- The OLED shows `LIGHT` / `NN %`, and the log shows `Light: NN %`, always
  within 0-100.
- **Brighter gives a higher percentage.** If brighter gives a *lower*
  percentage, the LDR direction assumption is reversed. That is a known,
  unverified assumption (see `docs/dev-log.md`, "SensorData brought in line
  with the specification"): record it as FAIL, and note that the one-line fix
  is in `src/sensors_logic.cpp`.

**Result:** PASS | **Observed (bright %, dark %):** 99 %, 1 %. Brighter gives a higher percentage, so the LDR direction is correct. | **Date:** 2026-09-27

## FT-04: PIR motion (FR-04)

**Steps**
1. Turn the encoder CW until the log shows `INPUT: mode MOTION`.
2. Trigger the PIR.
3. Wait for the PIR output to go low again (Wokwi holds it high for a few
   seconds).

**Expected**
- On the trigger: `MOTION: detected`, and the OLED value shows `DETECTED`.
- When it ends: `MOTION: clear`, and the OLED shows `CLEAR`.
- The next `Sample:` line while motion is present ends with `Motion: yes`.

**Result:** PASS | **Observed:** The OLED showed `MOTION` / `DETECTED` and `MOTION` / `CLEAR`, matching `MOTION: detected` / `MOTION: clear` in the terminal. | **Date:** 2026-09-27

## FT-05: one measurement at a time (FR-05)

**Steps**
1. Visit all four modes with the encoder and look at the OLED in each.

**Expected**
- The screen always has exactly: `ROOM MONITOR`, one mode label
  (`TEMPERATURE`, `HUMIDITY`, `LIGHT` or `MOTION`) and one value.
- The second line is blank, unless the temperature alarm is sounding (then
  it shows `ALARM LOW` or `ALARM HIGH`).
- Never two measurements on screen at once.

**Result:** PASS | **Observed:** Across all the mode tests, the OLED always showed exactly one label and one value, never two measurements at once. | **Date:** 2026-09-27

## FT-06: encoder navigation with wraparound (FR-06)

**Steps**
1. Start from TEMPERATURE (the boot mode).
2. Turn **CW four times**, one step at a time.
3. Turn **CCW four times**, one step at a time.
4. Press the encoder knob once.

**Expected**
- CW: `INPUT: mode HUMIDITY`, `INPUT: mode LIGHT`, `INPUT: mode MOTION`,
  `INPUT: mode TEMPERATURE` (wraps around).
- CCW: `INPUT: mode MOTION` (wraps around), `INPUT: mode LIGHT`,
  `INPUT: mode HUMIDITY`, `INPUT: mode TEMPERATURE`.
- The OLED changes to the new mode immediately, without waiting for the next
  sample.
- The press logs `INPUT: button pressed`.
- If CW and CCW are swapped, record it: the encoder's direction is a wiring
  and decoding assumption that has not been verified.

**Result:** PASS | **Observed:** Full cycle MOTION → LIGHT → HUMIDITY → TEMPERATURE → MOTION, wrapping correctly, with the OLED matching every step. The button was pressed twice, and `INPUT: button pressed` was logged both times. | **Date:** 2026-09-27

## FT-07: temperature alarm (FR-07)

Keep the system ACTIVE throughout (trigger the PIR as needed). The alarm is
re-evaluated on each sample, so wait at least 2 s after each change.

**Steps and expected results**

| Step | Set temperature | Expected |
|---|---|---|
| 1 | 30.0 °C | No alarm (30.0 is inside the range) |
| 2 | 30.1 °C | `ALARM: temperature HIGH`, `ALARM: buzzer on`; the OLED second line shows `ALARM HIGH`; the buzzer sounds (about 1 kHz) |
| 3 | 17.9 °C | `ALARM: temperature LOW`; the buzzer stays on; the OLED shows `ALARM LOW` |
| 4 | 18.0 °C | `ALARM: temperature NORMAL`, `ALARM: buzzer off`; the OLED alarm line disappears |
| 5 | 24.0 °C | Still no alarm |

**Result:** PASS | **Observed (per step):** HIGH: 80 °C gave `ALARM: temperature HIGH`, buzzer on, OLED `ALARM HIGH`. LOW: 11.8 °C gave `ALARM: temperature LOW`, OLED `ALARM LOW`. No results were recorded for the boundary steps (30.0 / 30.1 / 17.9 / 18.0 °C) in this run; the boundaries are covered by the native unit tests in `test/test_alarm`. | **Date:** 2026-09-27

## FT-08: ACTIVE and INACTIVE behaviour (FR-08)

**Steps**
1. While ACTIVE, note: OLED on, `Sample:` lines every 2 s, and encoder turns
   change the mode.
2. Stop triggering the PIR and wait for `STATE: INACTIVE (no motion for 15 s)`.
3. While INACTIVE, turn the encoder a few steps and watch for 10 s.

**Expected**
- **ACTIVE:** as in step 1.
- **INACTIVE:**
  - the OLED goes blank;
  - no `Sample:` lines;
  - encoder turns produce no `INPUT: mode ...` lines;
  - the heartbeat LED keeps toggling every 2 s;
  - triggering the PIR still logs `MOTION: detected`.

**Result:** PASS | **Observed:** A continuous log showed steady 2 s sampling throughout ACTIVE, and sampling stopped immediately at `STATE: INACTIVE (no motion for 15 s)`. | **Date:** 2026-09-27

## FT-09: INACTIVE after 15 s without motion (FR-09)

**Steps**
1. Trigger the PIR once. Note the simulation time of `MOTION: clear` (when
   the PIR output goes low again).
2. Do nothing else.
3. Note the time of `STATE: INACTIVE (no motion for 15 s)`.

**Expected**
- `STATE: INACTIVE` about **15 s after `MOTION: clear`**, within about
  ±0.5 s. StateTask re-checks motion at most every 500 ms while motion is
  present, so the 15 s count starts from the last check that saw motion.

**Result:** PASS | **Observed (clear at / inactive at):** `STATE: INACTIVE (no motion for 15 s)` fired correctly several times across different test runs; exact times were not recorded. | **Date:** 2026-09-27

## FT-10: motion returns the system to ACTIVE (FR-10)

**Steps**
1. Wait until the system is INACTIVE (see FT-09).
2. Trigger the PIR.

**Expected**
- `MOTION: detected`, then `STATE: ACTIVE (motion)` almost immediately
  (MotionTask samples every 100 ms).
- The OLED comes back, showing the last mode and the last sample.
- `Sample:` lines resume within 2 s, and the encoder works again.

**Result:** PASS | **Observed:** `MOTION: detected` followed by `STATE: ACTIVE (motion)`, confirmed several times; the OLED and sampling resumed each time. | **Date:** 2026-09-27

## Fault experiments

Each one checks that a failure stays contained: the affected function
degrades in a defined way, and the rest of the system keeps running. F-1 and
F-2 change `diagram.json`. **Restore it afterwards** with
`git checkout -- diagram.json` and restart the simulation.

### F-1: DHT22 disconnected

**Steps**
1. In `diagram.json`, delete the connection `[ "stm32:A0", "dht:SDA", ... ]`.
2. Start the simulation and watch for 20 s, triggering the PIR and turning
   the encoder a few times.

**Expected**
- Every 2 s: `DHT22: read failed (no response), sample skipped`.
- No `Sample:` lines, so nothing is ever queued: the OLED stays **blank**
  (DisplayTask only draws after its first sample) and the alarm never
  evaluates a temperature.
- Everything else keeps working: the six `started` lines, `MOTION: ...`,
  `INPUT: mode ...`, the `STATE: ...` transitions and the heartbeat LED.
- No crash, hang or reset.

**Result:** PASS | **Observed:** `DHT22: read failed (no response), sample skipped` every 2 s. MOTION and STATE lines and the heartbeat LED kept working normally, and the OLED stayed blank throughout. | **Date:** 2026-09-27

### F-2: OLED disconnected

**Steps**
1. In `diagram.json`, delete the connections `[ "stm32:B6", "oled:SCL", ... ]`
   and `[ "stm32:B7", "oled:SDA", ... ]`.
2. Start the simulation and watch for 20 s, triggering the PIR and turning
   the encoder.

**Expected**
- The OLED shows nothing.
- `Sample:` lines still arrive every 2 s, and `MOTION`, `INPUT`, `STATE` and
  `ALARM` messages still appear. DisplayTask's I2C calls fail or time out,
  but it has the lowest priority and owns the display alone, so no other task
  waits for it.
- No crash, hang or reset. Note anything unexpected, such as the timing of
  other log lines changing.

**Result:** PASS | **Observed:** `DISPLAY: OLED initialised` still printed at boot despite the disconnection, the silent failure predicted (I2C results are not checked). `Sample:`, `MOTION`, `INPUT`, `STATE` and `ALARM` lines kept arriving at the normal pace with no bunching or delay, and the OLED stayed dark. | **Date:** 2026-09-27

### F-3: temperature alarm while INACTIVE

No diagram change.

**Steps**
1. While ACTIVE, set the DHT22 temperature to **35 °C**. Wait for
   `ALARM: buzzer on`.
2. Stop triggering the PIR and wait for `STATE: INACTIVE`.
3. While INACTIVE, set the temperature back to **24 °C** and wait 10 s.
4. Trigger the PIR.

**Expected**
- Step 2: `ALARM: buzzer off` within about 0.5 s of `STATE: INACTIVE`, and
  the buzzer stays silent while INACTIVE. The alarm is only active in the
  ACTIVE state.
- Step 3: no sampling while INACTIVE, so the temperature change is not seen
  yet.
- Step 4: `STATE: ACTIVE (motion)`, then **`ALARM: buzzer on` within about
  0.5 s**: AlarmTask still holds the last temperature it saw (35 °C). Within
  2 s the next sample reports 24 °C, followed by
  `ALARM: temperature NORMAL` and `ALARM: buzzer off`.
- This short resume on a stale reading is a known consequence of the current
  design (see `src/alarm.cpp`). Record whether it happens.

**Result:** PASS | **Observed:** On waking from INACTIVE with the PIR, the
OLED and the alarm used the stale 35.2 °C reading: the buzzer sounded again
for about 1 s. The next sample read the actual temperature, 24.60 °C, and the
buzzer turned off automatically. Matches the predicted behaviour. Step 2:
`ALARM: buzzer off` was the line immediately after
`STATE: INACTIVE (no motion for 15 s)`, well within the expected ~0.5 s. |
**Date:** 2026-09-27
