#include <unity.h>

#include "display_logic.h"

/* Unity compares integers, so compare the enum's underlying values. */
#define ASSERT_MODE(expected, actual) \
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(expected), (uint8_t)(actual))

void setUp(void) {}
void tearDown(void) {}

/* Clockwise, one step at a time, without wrapping. */
static void test_next_moves_forward(void) {
    ASSERT_MODE(DisplayMode::HUMIDITY, nextDisplayMode(DisplayMode::TEMPERATURE));
    ASSERT_MODE(DisplayMode::LIGHT, nextDisplayMode(DisplayMode::HUMIDITY));
    ASSERT_MODE(DisplayMode::MOTION, nextDisplayMode(DisplayMode::LIGHT));
}

/* Counterclockwise, one step at a time, without wrapping. */
static void test_previous_moves_backward(void) {
    ASSERT_MODE(DisplayMode::LIGHT, previousDisplayMode(DisplayMode::MOTION));
    ASSERT_MODE(DisplayMode::HUMIDITY, previousDisplayMode(DisplayMode::LIGHT));
    ASSERT_MODE(DisplayMode::TEMPERATURE, previousDisplayMode(DisplayMode::HUMIDITY));
}

/* Clockwise past the last mode goes back to the first. */
static void test_next_wraps_from_motion_to_temperature(void) {
    ASSERT_MODE(DisplayMode::TEMPERATURE, nextDisplayMode(DisplayMode::MOTION));
}

/* Counterclockwise past the first mode goes to the last. */
static void test_previous_wraps_from_temperature_to_motion(void) {
    ASSERT_MODE(DisplayMode::MOTION, previousDisplayMode(DisplayMode::TEMPERATURE));
}

static const DisplayMode kAllModes[] = {
    DisplayMode::TEMPERATURE, DisplayMode::HUMIDITY, DisplayMode::LIGHT, DisplayMode::MOTION,
};

/* One detent back and forth returns to the same mode, from every mode. */
static void test_next_and_previous_are_inverse(void) {
    for (DisplayMode mode : kAllModes) {
        ASSERT_MODE(mode, previousDisplayMode(nextDisplayMode(mode)));
        ASSERT_MODE(mode, nextDisplayMode(previousDisplayMode(mode)));
    }
}

/* Four detents in either direction is a full cycle through every mode. */
static void test_four_steps_is_a_full_cycle(void) {
    for (DisplayMode start : kAllModes) {
        DisplayMode cw = start;
        DisplayMode ccw = start;
        for (int i = 0; i < 4; i++) {
            cw = nextDisplayMode(cw);
            ccw = previousDisplayMode(ccw);
        }
        ASSERT_MODE(start, cw);
        ASSERT_MODE(start, ccw);
    }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_next_moves_forward);
    RUN_TEST(test_previous_moves_backward);
    RUN_TEST(test_next_wraps_from_motion_to_temperature);
    RUN_TEST(test_previous_wraps_from_temperature_to_motion);
    RUN_TEST(test_next_and_previous_are_inverse);
    RUN_TEST(test_four_steps_is_a_full_cycle);
    return UNITY_END();
}
