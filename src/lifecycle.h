#ifndef EYEKI_LIFECYCLE_H
#define EYEKI_LIFECYCLE_H

#include <stdbool.h>

typedef enum {
    POPUP_CLOSE_NONE,
    POPUP_CLOSE_ACKNOWLEDGED,
    POPUP_CLOSE_WINDOW_MANAGER,
    POPUP_CLOSE_MODE_CHANGE,
    POPUP_CLOSE_SHUTDOWN
} PopupCloseReason;

typedef struct {
    bool open;
    bool close_pending;
    PopupCloseReason close_reason;
} PopupLifecycle;

void popup_lifecycle_init(PopupLifecycle *lifecycle);
bool popup_lifecycle_open(PopupLifecycle *lifecycle);
bool popup_lifecycle_dismiss(
    PopupLifecycle *lifecycle,
    PopupCloseReason reason
);
bool popup_lifecycle_is_open(const PopupLifecycle *lifecycle);
bool popup_lifecycle_take_close(
    PopupLifecycle *lifecycle,
    PopupCloseReason *reason
);

bool lifecycle_install_signal_handlers(void);
bool lifecycle_shutdown_requested(void);

#endif /* EYEKI_LIFECYCLE_H */
