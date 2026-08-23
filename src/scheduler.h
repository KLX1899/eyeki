#ifndef EYEKI_SCHEDULER_H
#define EYEKI_SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef enum {
    ACTIVITY_ACTIVE,
    ACTIVITY_IDLE,
    ACTIVITY_UNKNOWN
} ActivityState;

typedef struct {
    uint64_t interval_nanoseconds;
    uint64_t active_nanoseconds;
    struct timespec last_sample;
} Scheduler;

bool scheduler_init(
    Scheduler *scheduler,
    uint64_t interval_seconds,
    struct timespec now
);
bool scheduler_record_sample(
    Scheduler *scheduler,
    ActivityState activity,
    struct timespec now
);
void scheduler_reset(Scheduler *scheduler, struct timespec now);
uint64_t scheduler_active_seconds(const Scheduler *scheduler);

#endif /* EYEKI_SCHEDULER_H */
