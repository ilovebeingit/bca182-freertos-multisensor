#include "sensors_logic.h"

int lightPercentFromAdc(uint16_t raw) {
    uint32_t clamped = raw > kAdcMax ? kAdcMax : raw;
    uint32_t darkness = kAdcMax - clamped;              /* bright -> large */
    return (int)((darkness * 100U + kAdcMax / 2U) / kAdcMax);
}
