#include <unity.h>

#include "alarm_logic.h"

/* Unity compares integers, so compare the enum's underlying values. */
#define ASSERT_ALARM(expected, actual) \
    TEST_ASSERT_EQUAL_INT((int)(expected), (int)(actual))

void setUp(void) {}
void tearDown(void) {}

/* FR-07: below 18.0 degC the alarm is LOW_TEMPERATURE. */
static void test_below_low_limit_is_low_temperature(void) {
    ASSERT_ALARM(AlarmState::LOW_TEMPERATURE, evaluateTemperature(17.9f));
}

/* 18.0 degC is inside the 18.0-30.0 range, so NORMAL. */
static void test_exactly_low_limit_is_normal(void) {
    ASSERT_ALARM(AlarmState::NORMAL, evaluateTemperature(18.0f));
}

/* A typical room temperature is NORMAL. */
static void test_normal_temperature_is_normal(void) {
    ASSERT_ALARM(AlarmState::NORMAL, evaluateTemperature(24.0f));
}

/* 30.0 degC is inside the 18.0-30.0 range, so NORMAL. */
static void test_exactly_high_limit_is_normal(void) {
    ASSERT_ALARM(AlarmState::NORMAL, evaluateTemperature(30.0f));
}

/* Above 30.0 degC the alarm is HIGH_TEMPERATURE. */
static void test_above_high_limit_is_high_temperature(void) {
    ASSERT_ALARM(AlarmState::HIGH_TEMPERATURE, evaluateTemperature(30.1f));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_below_low_limit_is_low_temperature);
    RUN_TEST(test_exactly_low_limit_is_normal);
    RUN_TEST(test_normal_temperature_is_normal);
    RUN_TEST(test_exactly_high_limit_is_normal);
    RUN_TEST(test_above_high_limit_is_high_temperature);
    return UNITY_END();
}
