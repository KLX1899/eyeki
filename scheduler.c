#include "scheduler.h"

#include <limits.h>

#define NANOSECONDS_PER_SECOND 1000000000ULL

static bool timespec_difference(
    struct timespec newer,
    struct timespec older,
    uint64_t *difference
) {
    time_t seconds;
    long nanoseconds;

    if (newer.tv_nsec < 0 || newer.tv_nsec >= (long)NANOSECONDS_PER_SECOND ||
        older.tv_nsec < 0 || older.tv_nsec >= (long)NANOSECONDS_PER_SECOND) {
        return false;
    }

    if (newer.tv_sec < older.tv_sec ||
        (newer.tv_sec == older.tv_sec && newer.tv_nsec < older.tv_nsec)) {
        return false;
    }

    seconds = newer.tv_sec - older.tv_sec;
    nanoseconds = newer.tv_nsec - older.tv_nsec;
    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += (long)NANOSECONDS_PER_SECOND;
    }

    if ((uint64_t)seconds > UINT64_MAX / NANOSECONDS_PER_SECOND) {
        return false;
    }

    *difference = (uint64_t)seconds * NANOSECONDS_PER_SECOND;
    if (*difference > UINT64_MAX - (uint64_t)nanoseconds) {
        return false;
    }
    *difference += (uint64_t)nanoseconds;
    return true;
}

bool scheduler_init(
    Scheduler *scheduler,
    uint64_t interval_seconds,
    struct timespec now
) {
    if (!scheduler || interval_seconds == 0 ||
        interval_seconds > UINT64_MAX / NANOSECONDS_PER_SECOND) {
        return false;
    }

    scheduler->interval_nanoseconds =
        interval_seconds * NANOSECONDS_PER_SECOND;
    scheduler->active_nanoseconds = 0;
    scheduler->last_sample = now;
    return true;
}

bool scheduler_record_sample(
    Scheduler *scheduler,
    ActivityState activity,
    struct timespec now
) {
    uint64_t elapsed;

    if (!scheduler ||
        !timespec_difference(now, scheduler->last_sample, &elapsed)) {
        if (scheduler) {
            scheduler_reset(scheduler, now);
        }
        return false;
    }

    scheduler->last_sample = now;

    if (activity != ACTIVITY_ACTIVE) {
        scheduler->active_nanoseconds = 0;
        return false;
    }

    if (scheduler->active_nanoseconds > UINT64_MAX - elapsed) {
        scheduler->active_nanoseconds = scheduler->interval_nanoseconds;
    } else {
        scheduler->active_nanoseconds += elapsed;
    }

    if (scheduler->active_nanoseconds >= scheduler->interval_nanoseconds) {
        scheduler->active_nanoseconds = 0;
        return true;
    }

    return false;
}

void scheduler_reset(Scheduler *scheduler, struct timespec now) {
    if (!scheduler) {
        return;
    }

    scheduler->active_nanoseconds = 0;
    scheduler->last_sample = now;
}

uint64_t scheduler_active_seconds(const Scheduler *scheduler) {
    if (!scheduler) {
        return 0;
    }

    return scheduler->active_nanoseconds / NANOSECONDS_PER_SECOND;
}
