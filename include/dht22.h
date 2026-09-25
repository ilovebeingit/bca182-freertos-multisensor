#pragma once

/* DHT22 on PA0, single-wire, bit-banged with DWT cycle-counter timing. */

#include "dht22_logic.h"

/* Enables the DWT cycle counter and releases PA0 (input, pull-up). */
void dht22_init(void);

/* Blocking read, about 7 ms. Must be called from a task (it uses vTaskDelay
 * for the start pulse). Writes *out only on DHT22_OK. */
Dht22Status dht22_read(Dht22Reading *out);
