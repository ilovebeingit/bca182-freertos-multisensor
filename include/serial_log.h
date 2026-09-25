#pragma once

/* USART1 logging: PA9 TX, 115200 8N1. */

void serial_log_init(void);

/* Writes msg followed by "\r\n". Once the scheduler is running, the whole
 * line is sent while holding serialMutex so lines from different tasks never
 * interleave or get dropped. Before the scheduler starts only main() runs, so
 * the mutex is not needed (and may not exist yet). Not for use in ISRs. */
void log_line(const char *msg);
