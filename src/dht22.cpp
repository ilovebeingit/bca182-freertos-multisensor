#include "dht22.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

static bool s_cycle_counter_ok = false;
static uint32_t s_cycles_per_us = 1;

/* PA0 mode switches by direct CRL writes: HAL_GPIO_Init takes tens of
 * microseconds at 8 MHz, too slow for the release-then-listen step. Only
 * init code writes GPIOA->CRL otherwise, so the read-modify-write is safe. */
static void pa0_release(void) {
    GPIOA->BSRR = GPIO_PIN_0;                               /* ODR=1: pull-up */
    GPIOA->CRL = (GPIOA->CRL & ~0xFUL) | 0x8UL;             /* input, pull-up/down */
}

static void pa0_drive_low(void) {
    GPIOA->BRR = GPIO_PIN_0;
    GPIOA->CRL = (GPIOA->CRL & ~0xFUL) | 0x6UL;             /* open-drain out, 2 MHz */
}

static inline bool pa0_is_high(void) {
    return (GPIOA->IDR & GPIO_PIN_0) != 0;
}

static inline uint32_t us_to_cycles(uint32_t us) {
    return us * s_cycles_per_us;
}

/* Waits until PA0 reads `high`, at most timeout_cycles. The iteration cap
 * (each pass takes several cycles, so it never fires first while the counter
 * runs) bounds the loop even if the cycle counter stops, so the critical
 * section can never hang. */
static bool wait_level(bool high, uint32_t timeout_cycles) {
    uint32_t start = DWT->CYCCNT;

    for (uint32_t guard = 0; guard < timeout_cycles; guard++) {
        if (pa0_is_high() == high) {
            return true;
        }
        if (DWT->CYCCNT - start > timeout_cycles) {
            return false;
        }
    }
    return false;
}

void dht22_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    pa0_release();

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    s_cycles_per_us = SystemCoreClock / 1000000U;

    uint32_t before = DWT->CYCCNT;
    for (volatile uint32_t i = 0; i < 16; i++) {}
    s_cycle_counter_ok = DWT->CYCCNT != before;
}

Dht22Status dht22_read(Dht22Reading *out) {
    uint16_t high_us[kDht22Bits];
    Dht22Status status = DHT22_OK;

    if (!s_cycle_counter_ok) {
        return DHT22_NO_TIMER;
    }

    /* Start signal: hold the line low for at least 1 ms. Not timing-critical
     * (up to 20 ms is allowed), so block normally and let other tasks run. */
    pa0_drive_low();
    vTaskDelay(pdMS_TO_TICKS(2));

    /* From release to the last bit (about 4-5 ms) every edge must be seen
     * within a few microseconds: a '0' and a '1' differ only in how long the
     * line stays high (26-28 us vs 70 us). If a tick, or a context switch
     * made from it, landed in the middle of a bit, that pulse would measure
     * too long and could be misread, so this stretch runs in a critical
     * section. With this project's FreeRTOS port that gates the SysTick
     * interrupt (the only interrupt in the system) and, when the section
     * ends, catches up the ticks that fell inside it, so the RTOS tick count
     * stays right even though this lasts several ticks. Only pulse widths
     * are captured here; decoding and logging happen after. */
    taskENTER_CRITICAL();

    pa0_release();

    /* Line rises (pull-up), sensor pulls low 80 us, high 80 us, then low
     * for the first bit. */
    if (!wait_level(true, us_to_cycles(100)) ||
        !wait_level(false, us_to_cycles(100)) ||
        !wait_level(true, us_to_cycles(100)) ||
        !wait_level(false, us_to_cycles(100))) {
        status = DHT22_NO_RESPONSE;
    } else {
        for (uint8_t i = 0; i < kDht22Bits; i++) {
            if (!wait_level(true, us_to_cycles(80))) {      /* 50 us low */
                status = DHT22_TIMEOUT;
                break;
            }
            uint32_t rise = DWT->CYCCNT;
            if (!wait_level(false, us_to_cycles(100))) {    /* 26-28 or 70 us high */
                status = DHT22_TIMEOUT;
                break;
            }
            high_us[i] = (uint16_t)((DWT->CYCCNT - rise) / s_cycles_per_us);
        }
    }

    taskEXIT_CRITICAL();

    if (status != DHT22_OK) {
        return status;
    }
    return dht22_decode(high_us, out);
}
