#include "input_logic.h"

int8_t encoder_step(bool prev_clk_high, bool clk_high, bool dt_high) {
    if (prev_clk_high && !clk_high) {
        return (dt_high != clk_high) ? 1 : -1;
    }
    return 0;
}

bool button_is_pressed(bool sw_high) {
    return !sw_high;
}
