#include <unity.h>

#include "system_state.h"

/* Unity compares integers, so compare the enum's underlying values. */
#define ASSERT_STATE(expected, actual) \
    TEST_ASSERT_EQUAL_INT((int)(expected), (int)(actual))

void setUp(void) {}
void tearDown(void) {}

/* ACTIVE and the 15 s have not run out: stay ACTIVE. */
static void test_active_without_timeout_stays_active(void) {
    ASSERT_STATE(SystemState::ACTIVE,
                 evaluateSystemState(SystemState::ACTIVE, false, false));
}

/* FR-09: ACTIVE with no motion for 15 s becomes INACTIVE. */
static void test_active_with_timeout_becomes_inactive(void) {
    ASSERT_STATE(SystemState::INACTIVE,
                 evaluateSystemState(SystemState::ACTIVE, false, true));
}

/* INACTIVE and still no motion: stay INACTIVE. */
static void test_inactive_without_motion_stays_inactive(void) {
    ASSERT_STATE(SystemState::INACTIVE,
                 evaluateSystemState(SystemState::INACTIVE, false, true));
}

/* FR-10: motion wakes an INACTIVE system. */
static void test_inactive_with_motion_becomes_active(void) {
    ASSERT_STATE(SystemState::ACTIVE,
                 evaluateSystemState(SystemState::INACTIVE, true, true));
}

/* Motion seen in the same evaluation as an elapsed timeout wins: the system
 * must not go INACTIVE while someone is moving in front of the PIR. */
static void test_motion_overrides_elapsed_timeout(void) {
    ASSERT_STATE(SystemState::ACTIVE,
                 evaluateSystemState(SystemState::ACTIVE, true, true));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_active_without_timeout_stays_active);
    RUN_TEST(test_active_with_timeout_becomes_inactive);
    RUN_TEST(test_inactive_without_motion_stays_inactive);
    RUN_TEST(test_inactive_with_motion_becomes_active);
    RUN_TEST(test_motion_overrides_elapsed_timeout);
    return UNITY_END();
}
