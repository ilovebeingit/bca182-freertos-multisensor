#include "alarm_logic.h"

bool alarm_should_sound(uint8_t motion_flag) {
    return motion_flag != 0;
}
