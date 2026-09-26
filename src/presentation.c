#include "presentation.h"

bool presentation_backends_init(
    PresentationBackends *backends,
    PresentationBackendInitializer initialize_notification,
    void *notification_context,
    PresentationBackendInitializer initialize_popup,
    void *popup_context
) {
    if (!backends || !initialize_notification || !initialize_popup) {
        return false;
    }

    backends->initialize_notification = initialize_notification;
    backends->notification_context = notification_context;
    backends->initialize_popup = initialize_popup;
    backends->popup_context = popup_context;
    backends->notification_ready = false;
    backends->popup_ready = false;
    return true;
}

bool presentation_backends_prepare(
    PresentationBackends *backends,
    ReminderMode mode
) {
    bool initialized;

    if (!backends) {
        return false;
    }

    if (mode == MODE_NOTIFICATION) {
        if (backends->notification_ready) {
            return true;
        }
        if (!backends->initialize_notification) {
            return false;
        }
        initialized = backends->initialize_notification(
            backends->notification_context
        );
        if (initialized) {
            backends->notification_ready = true;
        }
        return initialized;
    }

    if (mode == MODE_POPUP) {
        if (backends->popup_ready) {
            return true;
        }
        if (!backends->initialize_popup) {
            return false;
        }
        initialized = backends->initialize_popup(backends->popup_context);
        if (initialized) {
            backends->popup_ready = true;
        }
        return initialized;
    }

    return false;
}

bool presentation_backend_is_ready(
    const PresentationBackends *backends,
    ReminderMode mode
) {
    if (!backends) {
        return false;
    }
    if (mode == MODE_NOTIFICATION) {
        return backends->notification_ready;
    }
    if (mode == MODE_POPUP) {
        return backends->popup_ready;
    }
    return false;
}
