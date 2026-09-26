#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#include "alarm.h"
#include "display.h"
#include "input.h"
#include "motion.h"
#include "rtos_objects.h"
#include "sensors.h"
#include "serial_log.h"
#include "system_state.h"

static void SystemClock_Config(void);
void app_main(void);

extern "C" void xPortSysTickHandler(void);

/* HAL_Init() starts SysTick long before the scheduler exists. Always advance
 * the HAL tick (uwTick, which the default HAL_GetTick reads) so HAL timeouts
 * work before and after the scheduler starts, and only hand the tick to
 * FreeRTOS once the kernel is running. extern "C" so it overrides the weak
 * vector-table symbol. */
extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}

/* MCU bring-up only, then hand over to the application. */
int main(void) {
    /* SystemInit() leaves VTOR at its reset value 0. FreeRTOS reads the initial
     * MSP from *VTOR when starting the first task, which only works where 0x0
     * aliases flash, so point VTOR at the flash vector table explicitly. */
    SCB->VTOR = FLASH_BASE;
    HAL_Init();
    SystemClock_Config();

    app_main();

    /* app_main() only returns if startup failed (and it has logged why). */
    for (;;) {}
}

/* Application entry (CLAUDE.md): hardware init -> RTOS objects -> tasks ->
 * scheduler. Does not return once the scheduler is running. */
void app_main(void) {
    serial_log_init();
    log_line("BCA182 FreeRTOS Multisensor");
    log_line("System starting...");

    sensors_init();
    motion_init();
    alarm_init();
    input_init();
    display_init();

    if (!rtos_objects_create()) {
        log_line("FATAL: could not create RTOS objects (FreeRTOS heap)");
        return;
    }

    bool created =
        xTaskCreate(SensorTask,  "SensorTask",  256, NULL, 2, NULL) == pdPASS &&
        xTaskCreate(DisplayTask, "DisplayTask", 256, NULL, 1, NULL) == pdPASS &&
        xTaskCreate(InputTask,   "InputTask",   128, NULL, 3, NULL) == pdPASS &&
        xTaskCreate(MotionTask,  "MotionTask",  128, NULL, 3, NULL) == pdPASS &&
        xTaskCreate(AlarmTask,   "AlarmTask",   128, NULL, 2, NULL) == pdPASS &&
        xTaskCreate(StateTask,   "StateTask",   128, NULL, 2, NULL) == pdPASS;
    if (!created) {
        log_line("FATAL: could not create all tasks (FreeRTOS heap)");
        return;
    }

    vTaskStartScheduler();

    /* Only reached if the idle task could not be created. */
    log_line("FATAL: scheduler did not start (FreeRTOS heap)");
}

/* Runs from the 8 MHz HSI reset clock (SystemCoreClock = 8000000). */
static void SystemClock_Config(void) {}
