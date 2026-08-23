#ifndef EYEKI_RUNTIME_H
#define EYEKI_RUNTIME_H

#include "config.h"
#include "scheduler.h"

typedef struct {
    Config config;
    Scheduler scheduler;
} RuntimeState;

bool runtime_state_init(
    RuntimeState *state,
    Config config,
    struct timespec now
);
bool runtime_state_reload(
    RuntimeState *state,
    Config config,
    struct timespec now
);

#endif /* EYEKI_RUNTIME_H */
