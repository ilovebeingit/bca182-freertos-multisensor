#include "system_state.h"

SystemState evaluateSystemState(SystemState current, bool motionDetected, bool timeoutElapsed) {
    if (motionDetected) {
        return SystemState::ACTIVE;
    }
    if (current == SystemState::ACTIVE && timeoutElapsed) {
        return SystemState::INACTIVE;
    }
    return current;
}
