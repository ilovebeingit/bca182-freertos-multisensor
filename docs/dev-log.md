# Development log

## 2026-09-25: FreeRTOS never starts the first task in Wokwi

### Symptom
In the Wokwi simulation only the boot banner printed. No task ever ran: no
"SensorTask started" line, no heartbeat, blank OLED. A GDB snapshot of the
running firmware showed:

- PC looping in `prvTaskExitError`, called from `xPortStartScheduler`, in
  Thread mode (IPSR 0), with no fault
- `xSchedulerRunning = 1`, all tasks created, `pxCurrentTCB` = "InputTask"
- PSP = `0xfffffffc` and CONTROL = 0, so the first task was never switched to
- `xTickCount` still increasing, so SysTick interrupts were being handled

So `vTaskStartScheduler()` reached `prvPortStartFirstTask()`, but its
`svc 0` never entered `vPortSVCHandler`. Execution carried on past it into the
"should never get here" path.

### Ruled out
- The vector table: SVC, PendSV and SysTick resolve to FreeRTOS's port.o and
  our SysTick wrapper. Checked with nm, a linker map and disassembly; there is
  no `stm32f1xx_it.c` override.
- VTOR / the initial MSP (the loaded MSP was the correct `0x20005000`), the
  `clz` instruction (optimised task selection), interrupt-priority config,
  heap and hooks.
- Application code: a one-task smoke build failed the same way.

### Probe (firmware reports its own state; GDB reads of PRIMASK/SCB proved unreliable)
A one-off build replicated `prvPortStartFirstTask` instruction for
instruction (`msr msp` from `*(*VTOR)`, `cpsie i`, `cpsie f`, `dsb`, `isb`),
with BASEPRI = 0x50 and the PendSV/SysTick priorities as FreeRTOS leaves them.
It read the masks with CMSIS `__get_*()` right before `svc 0`, and used its own
counting SVC handler.

Run 1:

```
after cpsie i/f: PRIMASK=1 FAULTMASK=1 BASEPRI=0x00 CONTROL=0 MSP=0x20004f88
after svc: SVC_Handler entered 0 time(s)
```

Run 2 cleared the masks with `movs r0,#0; msr primask,r0; msr faultmask,r0`
before `svc 0`:

```
after cpsie i/f: PRIMASK=1 FAULTMASK=1 ...
after msr primask/faultmask=0: PRIMASK=0 FAULTMASK=0 BASEPRI=0x00
after svc: SVC_Handler entered 1 time(s), IPSR in handler=11
```

The firmware also read `VTOR = 0x08000000` and `SHPR3 = 0xF0000000`, so those
registers are emulated. GDB showing them as 0 was a limitation of the debugger
stub, not missing registers.

### Conclusion
In Wokwi's STM32 simulation, PRIMASK and FAULTMASK are set, not cleared, after
the `cpsie i` / `cpsie f` sequence. The ARMv7-M architecture requires CPSIE to
clear them. With the masks set, the SVC exception is not taken and the first
task never starts. MSR to PRIMASK/FAULTMASK works correctly.

### Workaround
`lib/freertos_port_patch/src/port.c` is a copy of the library's ARM_CM3
`port.c`, linked instead of the original. In `prvPortStartFirstTask()` it adds
`movs r0, #0; msr primask, r0; msr faultmask, r0` between `isb` and `svc 0`. On
real hardware `cpsie` has already cleared both masks, so the addition is a
no-op there.

### Result
With the workaround the first task starts in Wokwi ("MotionTask started"
prints), but nothing runs after it. See the next entry.

## 2026-09-25/26: context switches livelock in Wokwi; Wokwi's exception model

The remaining questions from the first entry were resolved by further
in-firmware probes. As before, values came from the firmware's own CMSIS reads,
recorded in RAM and read back; GDB was used only for plain memory reads and for
single-stepping.

### PendSV probe (FreeRTOS running, hooks at entry to and exit from PendSV)
- 148 157 entries and 148 157 exits. At every one: EXC_RETURN `0xFFFFFFFD`,
  PRIMASK = FAULTMASK = BASEPRI = 0, ICSR `0x0000000E` (PendSV active,
  PENDSVSET clear). So the pending bit is cleared on entry and the handler does
  return. The earlier "pending bit not cleared" guess was wrong.
- The handler alternated between MotionTask and InputTask, each always at the
  same stack depth. Each task's stacked return PC was its own yield store:
  MotionTask `0x08003372` (`str r2,[r3]` in `vTaskDelay`), InputTask
  `0x08002D6C` (`str.w r3,[r9]` in `xQueueSemaphoreTake`). The correct return
  address is the next instruction.
- **Cause:** when an exception is triggered by a store that writes
  ICSR.PENDSVSET, Wokwi stacks the store's own address as the return PC. The
  task resumes at the store, re-pends PendSV and switches again, forever. This
  also explains the standalone "PendSV never returns" result.

### Yield and BASEPRI probe (before FreeRTOS; a minimal two-task switcher using the same mechanism as the CM3 port)
```
BASEPRI write 0x50 -> read 0x00
SHPR3=0xf0000000 (0x40000000 with SysTick at 0x40)
SysTick ticks per busy loop: BASEPRI=0 -> 175, BASEPRI=0x50 -> 175, BASEPRI=0x50 & SysTick prio 0x40 -> 176
cpsid i -> PRIMASK=0, ticks per busy loop -> 176
PendSV pended under BASEPRI=0x50: taken while masked=1 ...; stacked PC = the pend store
CPS from clear: cpsie i->PRIMASK=1 cpsid i->0 cpsie i->1 | cpsie f->FAULTMASK=1 cpsid f->0 cpsie f->1
msr msp (same value) -> PRIMASK=0 FAULTMASK=0
```
Yield through `svc #1` (the SVC handler pends PendSV): one yield led to more
than 325 000 PendSV switches and no task progress. PendSV's EXC_RETURN was
`0xFFFFFFF1`, meaning it had preempted the SVC handler, and its stacked PC was
the ICSR store inside SVC_Handler. The SVC frame's own return PC was correct
(`yield_via_svc + 2`).

### Wokwi deviations from ARMv7-M found
1. **CPS is inverted:** `cpsie` sets PRIMASK/FAULTMASK and `cpsid` clears
   them. `msr msp` does not touch the masks. MSR to PRIMASK/FAULTMASK works.
2. **BASEPRI is not implemented:** it reads back 0 and masks nothing (SysTick
   and PendSV fire through BASEPRI = 0x50).
3. **Exception priorities are stored but not enforced:** SHPR values read back
   correctly, yet PendSV (0xF0) preempts SVC (0x00).
4. **Exceptions triggered by a store to ICSR stack the store's own address**
   as the return PC, so the store is executed again after the handler returns.
5. **A conditional MRS inside an IT block executes even when its condition is
   false** (`ite eq; mrseq r0,msp; mrsne r0,psp` with Z = 1 ended with
   r0 = PSP). Conditional LDR/STR were correctly skipped. The project firmware
   contains no conditional MRS/MSR, so this one does not affect it.

Not measured: whether PRIMASK = 1 blocks interrupts. The earlier FreeRTOS
snapshot showed SysTick still counting with PRIMASK reading 1.

### Consequences
- FreeRTOS context switches cannot work in Wokwi (deviation 4). Routing
  yields through SVC does not help (deviations 3 and 4), so that fix was not
  applied.
- In Wokwi, every BASEPRI critical section (kernel internals, mutexes, queues,
  `taskENTER_CRITICAL` in the DHT22 read) protects nothing (deviations 2 and 3).
- On real hardware all of this behaves as the architecture specifies. Nothing
  here points to a firmware or configuration bug.
- The `prvPortStartFirstTask` MSR workaround stays: it is correct and a no-op
  on hardware.
- Runtime verification is blocked in Wokwi until the scheduler issue is
  resolved; use real hardware (or an emulator with an accurate NVIC model).

### Still open
- Real hardware has not been tested.

## 2026-09-26: event groups are not built by default

Adding `xSystemEvents` needs FreeRTOS's `event_groups.c`, but the
STM32Cube Middleware-FreeRTOS library's build script compiles only the core
kernel files unless optional features are requested. Fix: add
`custom_freertos_features = event_groups` to `[env:bluepill_f103c8]` in
`platformio.ini`. Without it, every `xEventGroup*` call fails to link.

## 2026-09-26: SensorData brought in line with the specification

- `SensorData` now matches CLAUDE.md exactly: `temperature`, `humidity`,
  `int lightLevel` (0-100 %), `motionDetected`. The earlier `dht_valid` flag is
  gone. A failed DHT22 read now skips the whole cycle (nothing is queued, the
  reason is logged), so a consumer never receives a sample with invalid data.
  The heartbeat LED still toggles every period.
- **Unverified assumption, to be checked in Wokwi:** `lightPercentFromAdc()`
  assumes the LDR module's AO voltage *falls* as light rises (raw 0 -> 100 %,
  raw 4095 -> 0 %). If Wokwi shows the opposite (a brighter light giving a
  lower percentage), change `darkness = kAdcMax - clamped` to
  `darkness = clamped` in `src/sensors_logic.cpp` and swap the two endpoint
  tests.

## 2026-09-26: the MCU runs at 8 MHz, not 72 MHz

Before choosing the TIM1 prescaler for the 1 kHz buzzer PWM, the real clock
configuration was checked instead of assuming the Blue Pill's usual 72 MHz:
`SystemClock_Config()` is empty, `SystemInit()` links as a bare `bx lr`, no
RCC oscillator or clock configuration is linked, and `SystemCoreClock`'s
initial value in the image is 8 000 000. So the chip runs on its reset clock:
HSI 8 MHz with AHB and APB2 undivided, and TIM1 is clocked at 8 MHz.
(PlatformIO's "72MHz" is only the board description.)

For 1 kHz: PSC = 7 (1 MHz tick), ARR = 999, CCR = 500 (50 %).
`alarm_init()` computes PSC from `HAL_RCC_GetPCLK2Freq()` and the APB2
prescaler, so the tone stays at 1 kHz if a PLL is configured later, and it
logs the values it chose at boot (`ALARM: TIM1 clock ... Hz, PSC=..., ARR=...`).
The actual tone in Wokwi is still to be observed.

## 2026-09-26: waiting on a level event bit would spin

StateTask waits on EVENT_MOTION with the remaining part of the 15 s timeout.
EVENT_MOTION is a *level* bit: MotionTask keeps it set for as long as the PIR
reports motion, because DisplayTask and SensorTask read its current value.
A plain `xEventGroupWaitBits()` on a bit that stays set returns immediately
every time, which would turn StateTask into a busy loop during motion (against
the "every task must block" rule). Clearing the bit on exit would stop the
spinning, but other readers would then briefly see "no motion" while motion
is present. Fix: after seeing motion, StateTask blocks for a 500 ms hold-off
(`vTaskDelay`) before waiting again. Motion keeps the system ACTIVE with at
most 500 ms of added latency, well within the 15 s timeout.

## 2026-09-26: static analysis (pio check)

- cppcheck's `unusedFunction` produced 29 false positives per environment,
  because `pio check` runs cppcheck one file at a time and so cannot see calls
  made from other files. It is disabled for those runs (`check_flags`) and
  replaced by a whole-program run, which found no unused functions (and does
  catch a deliberately planted one).
- `// cppcheck-suppress` comments were ignored under `pio check`, although
  plain cppcheck 2.11 honoured them. The 12 C-style casts that come from
  CMSIS/FreeRTOS macros are therefore left visible and marked accepted. Full
  details are in `docs/static-analysis.md`.
