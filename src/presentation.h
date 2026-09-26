#ifndef EYEKI_PRESENTATION_H
#define EYEKI_PRESENTATION_H

#include <stdbool.h>

#include "config.h"

typedef bool (*PresentationBackendInitializer)(void *context);

typedef struct {
    PresentationBackendInitializer initialize_notification;
    void *notification_context;
    PresentationBackendInitializer initialize_popup;
    void *popup_context;
    bool notification_ready;
    bool popup_ready;
} PresentationBackends;

bool presentation_backends_init(
    PresentationBackends *backends,
    PresentationBackendInitializer initialize_notification,
    void *notification_context,
    PresentationBackendInitializer initialize_popup,
    void *popup_context
);
bool presentation_backends_prepare(
    PresentationBackends *backends,
    ReminderMode mode
);
bool presentation_backend_is_ready(
    const PresentationBackends *backends,
    ReminderMode mode
);

#endif /* EYEKI_PRESENTATION_H */
