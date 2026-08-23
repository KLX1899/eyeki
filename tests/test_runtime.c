#include "runtime.h"

#include <assert.h>
#include <stdio.h>

static struct timespec at_seconds(time_t seconds) {
    struct timespec value = { .tv_sec = seconds, .tv_nsec = 0 };
    return value;
}

static void test_interval_reload_restarts_from_zero(void) {
    RuntimeState state;
    Config initial = {60, MODE_POPUP};
    Config changed = {10, MODE_NOTIFICATION};

    assert(runtime_state_init(&state, initial, at_seconds(100)));
    assert(!scheduler_record_sample(
        &state.scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(400)
    ));
    assert(scheduler_active_seconds(&state.scheduler) == 300);

    assert(runtime_state_reload(&state, changed, at_seconds(400)));
    assert(state.config.interval_minutes == 10);
    assert(state.config.mode == MODE_NOTIFICATION);
    assert(scheduler_active_seconds(&state.scheduler) == 0);
    assert(!scheduler_record_sample(
        &state.scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(999)
    ));
    assert(scheduler_record_sample(
        &state.scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(1000)
    ));
}

static void test_mode_only_reload_restarts_from_zero(void) {
    RuntimeState state;
    Config initial = {10, MODE_POPUP};
    Config changed = {10, MODE_NOTIFICATION};

    assert(runtime_state_init(&state, initial, at_seconds(100)));
    assert(!scheduler_record_sample(
        &state.scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(699)
    ));
    assert(scheduler_active_seconds(&state.scheduler) == 599);

    assert(runtime_state_reload(&state, changed, at_seconds(699)));
    assert(state.config.mode == MODE_NOTIFICATION);
    assert(scheduler_active_seconds(&state.scheduler) == 0);
    assert(!scheduler_record_sample(
        &state.scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(700)
    ));
    assert(scheduler_record_sample(
        &state.scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(1299)
    ));
}

static void test_invalid_reload_preserves_complete_state(void) {
    RuntimeState state;
    Config initial = {10, MODE_POPUP};
    Config invalid = {9, MODE_NOTIFICATION};

    assert(runtime_state_init(&state, initial, at_seconds(100)));
    assert(!scheduler_record_sample(
        &state.scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(200)
    ));

    assert(!runtime_state_reload(&state, invalid, at_seconds(200)));
    assert(state.config.interval_minutes == 10);
    assert(state.config.mode == MODE_POPUP);
    assert(scheduler_active_seconds(&state.scheduler) == 100);
}

int main(void) {
    test_interval_reload_restarts_from_zero();
    test_mode_only_reload_restarts_from_zero();
    test_invalid_reload_preserves_complete_state();
    puts("runtime tests passed");
    return 0;
}
