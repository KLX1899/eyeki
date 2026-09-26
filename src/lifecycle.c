#include "lifecycle.h"

#include <signal.h>
#include <string.h>

static volatile sig_atomic_t shutdown_requested = 0;

static void request_shutdown(int signal_number) {
    (void)signal_number;
    shutdown_requested = 1;
}

void popup_lifecycle_init(PopupLifecycle *lifecycle) {
    if (!lifecycle) {
        return;
    }

    lifecycle->open = false;
    lifecycle->close_pending = false;
    lifecycle->close_reason = POPUP_CLOSE_NONE;
}

bool popup_lifecycle_open(PopupLifecycle *lifecycle) {
    if (!lifecycle || lifecycle->open || lifecycle->close_pending) {
        return false;
    }

    lifecycle->open = true;
    lifecycle->close_reason = POPUP_CLOSE_NONE;
    return true;
}

bool popup_lifecycle_dismiss(
    PopupLifecycle *lifecycle,
    PopupCloseReason reason
) {
    if (!lifecycle || !lifecycle->open ||
        reason <= POPUP_CLOSE_NONE || reason > POPUP_CLOSE_SHUTDOWN) {
        return false;
    }

    lifecycle->open = false;
    lifecycle->close_pending = true;
    lifecycle->close_reason = reason;
    return true;
}

bool popup_lifecycle_is_open(const PopupLifecycle *lifecycle) {
    return lifecycle && lifecycle->open;
}

bool popup_lifecycle_take_close(
    PopupLifecycle *lifecycle,
    PopupCloseReason *reason
) {
    if (!lifecycle || !reason || !lifecycle->close_pending) {
        return false;
    }

    *reason = lifecycle->close_reason;
    lifecycle->close_pending = false;
    lifecycle->close_reason = POPUP_CLOSE_NONE;
    return true;
}

bool lifecycle_install_signal_handlers(void) {
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = request_shutdown;
    if (sigemptyset(&action.sa_mask) < 0 ||
        sigaction(SIGINT, &action, NULL) < 0 ||
        sigaction(SIGTERM, &action, NULL) < 0) {
        return false;
    }
    return true;
}

bool lifecycle_shutdown_requested(void) {
    return shutdown_requested != 0;
}
