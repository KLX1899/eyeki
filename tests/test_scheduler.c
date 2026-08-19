#include "../scheduler.h"

#include <assert.h>
#include <stdio.h>

static struct timespec at_seconds(time_t seconds) {
    struct timespec value = { .tv_sec = seconds, .tv_nsec = 0 };
    return value;
}

static void test_active_time_reaches_interval(void) {
    Scheduler scheduler;

    assert(scheduler_init(&scheduler, 60, at_seconds(100)));
    assert(!scheduler_record_sample(
        &scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(130)
    ));
    assert(scheduler_active_seconds(&scheduler) == 30);
    assert(scheduler_record_sample(
        &scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(160)
    ));
    assert(scheduler_active_seconds(&scheduler) == 0);
}

static void test_idle_discards_active_time(void) {
    Scheduler scheduler;

    assert(scheduler_init(&scheduler, 60, at_seconds(100)));
    assert(!scheduler_record_sample(
        &scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(130)
    ));
    assert(!scheduler_record_sample(
        &scheduler,
        ACTIVITY_IDLE,
        at_seconds(140)
    ));
    assert(scheduler_active_seconds(&scheduler) == 0);
    assert(!scheduler_record_sample(
        &scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(170)
    ));
    assert(scheduler_active_seconds(&scheduler) == 30);
}

static void test_unknown_state_discards_active_time(void) {
    Scheduler scheduler;

    assert(scheduler_init(&scheduler, 60, at_seconds(100)));
    assert(!scheduler_record_sample(
        &scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(130)
    ));
    assert(!scheduler_record_sample(
        &scheduler,
        ACTIVITY_UNKNOWN,
        at_seconds(140)
    ));
    assert(scheduler_active_seconds(&scheduler) == 0);
}

static void test_backward_sample_resets_scheduler(void) {
    Scheduler scheduler;

    assert(scheduler_init(&scheduler, 60, at_seconds(100)));
    assert(!scheduler_record_sample(
        &scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(130)
    ));
    assert(!scheduler_record_sample(
        &scheduler,
        ACTIVITY_ACTIVE,
        at_seconds(120)
    ));
    assert(scheduler_active_seconds(&scheduler) == 0);
}

int main(void) {
    test_active_time_reaches_interval();
    test_idle_discards_active_time();
    test_unknown_state_discards_active_time();
    test_backward_sample_resets_scheduler();
    puts("scheduler tests passed");
    return 0;
}
