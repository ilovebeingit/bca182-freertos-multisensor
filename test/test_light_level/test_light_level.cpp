#include <unity.h>

#include "sensors_logic.h"

void setUp(void) {}
void tearDown(void) {}

/* AO at full scale means darkness: 0 %. */
static void test_full_scale_adc_is_zero_percent(void) {
    TEST_ASSERT_EQUAL_INT(0, lightPercentFromAdc(kAdcMax));
}

/* AO at 0 V means brightest: 100 %. */
static void test_zero_adc_is_hundred_percent(void) {
    TEST_ASSERT_EQUAL_INT(100, lightPercentFromAdc(0));
}

/* Mid-scale maps to the middle of the range, rounded to the nearest percent:
 * 2048 -> (2047 * 100) / 4095 = 49.99 -> 50. */
static void test_mid_scale_is_fifty_percent(void) {
    TEST_ASSERT_EQUAL_INT(50, lightPercentFromAdc(2048));
}

/* A 12-bit ADC cannot exceed 4095; anything larger is clamped, not wrapped. */
static void test_out_of_range_reading_is_clamped(void) {
    TEST_ASSERT_EQUAL_INT(0, lightPercentFromAdc(5000));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_full_scale_adc_is_zero_percent);
    RUN_TEST(test_zero_adc_is_hundred_percent);
    RUN_TEST(test_mid_scale_is_fifty_percent);
    RUN_TEST(test_out_of_range_reading_is_clamped);
    return UNITY_END();
}
