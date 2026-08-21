#include "runtime.h"

static bool configure_runtime_state(
    RuntimeState *state,
    Config config,
    struct timespec now
) {
    Scheduler scheduler;
    uint64_t interval_seconds;

    if (!state ||
        !interval_minutes_to_seconds(
            config.interval_minutes,
            &interval_seconds
        ) ||
        !scheduler_init(&scheduler, interval_seconds, now)) {
        return false;
    }

    state->config = config;
    state->scheduler = scheduler;
    return true;
}

bool runtime_state_init(
    RuntimeState *state,
    Config config,
    struct timespec now
) {
    return configure_runtime_state(state, config, now);
}

bool runtime_state_reload(
    RuntimeState *state,
    Config config,
    struct timespec now
) {
    return configure_runtime_state(state, config, now);
}
