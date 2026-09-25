#include "dht22_logic.h"

#include <stdio.h>

bool dht22_bit_from_high_us(uint32_t high_us) {
    return high_us > kDht22OneThresholdUs;
}

Dht22Status dht22_decode(const uint16_t high_us[kDht22Bits], Dht22Reading *out) {
    uint8_t bytes[5] = {0, 0, 0, 0, 0};

    for (uint8_t i = 0; i < kDht22Bits; i++) {
        bytes[i / 8] = (uint8_t)((bytes[i / 8] << 1) | (dht22_bit_from_high_us(high_us[i]) ? 1 : 0));
    }

    uint8_t sum = (uint8_t)(bytes[0] + bytes[1] + bytes[2] + bytes[3]);
    if (sum != bytes[4]) {
        return DHT22_CHECKSUM;
    }

    uint16_t humidity = (uint16_t)((bytes[0] << 8) | bytes[1]);
    int16_t temperature = (int16_t)(((bytes[2] & 0x7F) << 8) | bytes[3]);
    if (bytes[2] & 0x80) {
        temperature = (int16_t)-temperature;
    }

    if (humidity > 1000 || temperature < -400 || temperature > 800) {
        return DHT22_OUT_OF_RANGE;
    }

    out->temperature_tenths = temperature;
    out->humidity_tenths = humidity;
    return DHT22_OK;
}

void format_tenths_2dp(char *buf, size_t size, int32_t tenths) {
    const char *sign = tenths < 0 ? "-" : "";
    uint32_t magnitude = (uint32_t)(tenths < 0 ? -tenths : tenths);

    snprintf(buf, size, "%s%lu.%lu0", sign,
             (unsigned long)(magnitude / 10), (unsigned long)(magnitude % 10));
}

const char *dht22_status_name(Dht22Status status) {
    switch (status) {
        case DHT22_OK:           return "ok";
        case DHT22_NO_RESPONSE:  return "no response";
        case DHT22_TIMEOUT:      return "timeout";
        case DHT22_CHECKSUM:     return "checksum mismatch";
        case DHT22_OUT_OF_RANGE: return "value out of range";
        case DHT22_NO_TIMER:     return "cycle counter not running";
    }
    return "unknown";
}
