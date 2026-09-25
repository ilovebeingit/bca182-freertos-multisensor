#pragma once

/* DHT22 (AM2302) protocol decisions. Hardware-independent (no HAL/FreeRTOS):
 * the driver measures the 40 data-bit high pulses and everything else -
 * bit classification, checksum, range check, scaling, formatting - is here. */

#include <stddef.h>
#include <stdint.h>

constexpr uint8_t kDht22Bits = 40;

/* A '0' bit is high for 26-28 us, a '1' for about 70 us; split halfway. */
constexpr uint32_t kDht22OneThresholdUs = 48;

/* The sensor must not be read more often than every 2 s. */
constexpr uint32_t kDht22MinIntervalMs = 2000;

enum Dht22Status : uint8_t {
    DHT22_OK = 0,
    DHT22_NO_RESPONSE,      /* no response pulse after the start signal */
    DHT22_TIMEOUT,          /* a data bit edge never came */
    DHT22_CHECKSUM,         /* byte 4 != low byte of bytes 0..3 */
    DHT22_OUT_OF_RANGE,     /* checksum ok but values outside the sensor range */
    DHT22_NO_TIMER,         /* cycle counter not running; read not attempted */
};

struct Dht22Reading {
    int16_t temperature_tenths;   /* 0.1 degC, -400..800 */
    uint16_t humidity_tenths;     /* 0.1 %RH, 0..1000 */
};

bool dht22_bit_from_high_us(uint32_t high_us);

/* Decodes 40 high-pulse widths (us). Writes *out only when DHT22_OK. */
Dht22Status dht22_decode(const uint16_t high_us[kDht22Bits], Dht22Reading *out);

/* True when at least kDht22MinIntervalMs has passed since last_read_ms
 * (wrap-safe unsigned arithmetic). */
bool dht22_read_due(uint32_t now_ms, uint32_t last_read_ms);

/* "23.50", "-0.50": tenths printed with two decimals, no floating point. */
void format_tenths_2dp(char *buf, size_t size, int32_t tenths);

const char *dht22_status_name(Dht22Status status);
