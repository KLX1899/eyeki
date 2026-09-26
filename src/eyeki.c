#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <libnotify/notify.h>
#include <gtk/gtk.h>
#include "activity.h"
#include "config.h"
#include "config_watch.h"
#include "lifecycle.h"
#include "presentation.h"
#include "runtime.h"
#include "scheduler.h"
#include "version.h"

/* If the user has been idle for this long (in seconds), discard accumulated
   active time and require a fresh continuous-use interval. */
#define IDLE_THRESHOLD_SEC 60
#define ACTIVITY_SAMPLE_INTERVAL_SEC 10
#define POPUP_EVENT_INTERVAL_MILLISECONDS 50

typedef struct {
    int *argc;
    char ***argv;
} PopupBackendContext;

typedef struct {
    GtkWidget *window;
    PopupLifecycle lifecycle;
} Popup;

static bool initialize_notification_backend(void *context) {
    (void)context;

    if (notify_is_initted() || notify_init("EyeKi")) {
        return true;
    }

    fprintf(stderr,
        "Failed to initialize the notification backend. "
        "Check the desktop notification service and restart EyeKi.\n");
    return false;
}

static bool initialize_popup_backend(void *context) {
    PopupBackendContext *popup_context = context;

    if (gtk_init_check(popup_context->argc, popup_context->argv)) {
        return true;
    }

    fprintf(stderr,
        "Failed to initialize the popup backend: no GTK display is "
        "available. Restart EyeKi inside a graphical desktop session.\n");
    return false;
}

static bool reload_config(
    ConfigWatch *config_watch,
    RuntimeState *runtime,
    PresentationBackends *presentation,
    struct timespec now
) {
    bool changed;
    Config config;

    if (!config_watch_process(config_watch, &changed)) {
        return false;
    }
    if (!changed) {
        return true;
    }

    config = load_config();
    if (!presentation_backends_prepare(presentation, config.mode)) {
        fprintf(stderr,
            "Unable to apply the configuration reload because the selected "
            "presentation backend is unavailable. EyeKi will stop; correct "
            "the desktop-session problem and restart it.\n");
        errno = ENODEV;
        return false;
    }
    if (!runtime_state_reload(runtime, config, now)) {
        errno = EINVAL;
        return false;
    }
    return true;
}

static bool timespec_at_or_after(
    struct timespec value,
    struct timespec boundary
) {
    return value.tv_sec > boundary.tv_sec ||
        (value.tv_sec == boundary.tv_sec &&
         value.tv_nsec >= boundary.tv_nsec);
}

static int milliseconds_until(
    struct timespec deadline,
    struct timespec now
) {
    uint64_t milliseconds;
    time_t seconds;
    long nanoseconds;

    if (timespec_at_or_after(now, deadline)) {
        return 0;
    }

    seconds = deadline.tv_sec - now.tv_sec;
    nanoseconds = deadline.tv_nsec - now.tv_nsec;
    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000L;
    }

    if ((uint64_t)seconds > (uint64_t)INT_MAX / 1000ULL) {
        return INT_MAX;
    }
    milliseconds = (uint64_t)seconds * 1000ULL;
    milliseconds += ((uint64_t)nanoseconds + 999999ULL) / 1000000ULL;
    return milliseconds > INT_MAX ? INT_MAX : (int)milliseconds;
}

/* send_notification()
 *
 * Sends a desktop notification via libnotify.
 * The notification backend must already be initialized before invocation.
 * The notification auto-dismisses after 10 seconds (10000 ms).
 * Returns false and reports the libnotify error when delivery fails.
 */
static bool send_notification(void) {
    GError *error = NULL;
    NotifyNotification *n = notify_notification_new(
        "💧 وقت قطره چشم!",
        "یک ساعت از روشن بودن صفحه گذشته.\nالان از Artificial Tears استفاده کن.",
        "dialog-information"
    );

    if (!n) {
        fprintf(stderr,
            "Failed to create the reminder notification; the timer will "
            "restart and retry after the next interval.\n");
        return false;
    }

    notify_notification_set_urgency(n, NOTIFY_URGENCY_NORMAL);
    notify_notification_set_timeout(n, 10000); /* milliseconds */
    if (!notify_notification_show(n, &error)) {
        fprintf(stderr,
            "Failed to deliver the reminder notification%s%s; the timer "
            "will restart and retry after the next interval.\n",
            error && error->message ? ": " : "",
            error && error->message ? error->message : "");
        g_clear_error(&error);
        g_object_unref(G_OBJECT(n));
        return false;
    }
    g_object_unref(G_OBJECT(n));
    return true;
}

static void dismiss_popup(Popup *popup, PopupCloseReason reason) {
    if (!popup ||
        !popup_lifecycle_dismiss(&popup->lifecycle, reason)) {
        return;
    }

    if (popup->window) {
        gtk_widget_destroy(popup->window);
    }
}

static void on_button_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    dismiss_popup(data, POPUP_CLOSE_ACKNOWLEDGED);
}

static void on_popup_destroy(GtkWidget *widget, gpointer data) {
    Popup *popup = data;

    if (popup->window == widget) {
        popup->window = NULL;
    }
    if (popup_lifecycle_is_open(&popup->lifecycle)) {
        (void)popup_lifecycle_dismiss(
            &popup->lifecycle,
            POPUP_CLOSE_WINDOW_MANAGER
        );
    }
}

static void process_popup_events(const Popup *popup) {
    if (!popup_lifecycle_is_open(&popup->lifecycle)) {
        return;
    }

    while (gtk_events_pending()) {
        gtk_main_iteration_do(FALSE);
    }
}

/* show_popup()
 *
 * Builds and displays a fullscreen GTK overlay window. The daemon's main
 * loop continues servicing GTK, configuration, and process-lifecycle events
 * until the popup is dismissed.
 *
 * Layout:
 *   GtkWindow (fullscreen, always-on-top, undecorated)
 *   └── GtkOverlay
 *       ├── GtkDrawingArea  (semi-transparent dark background, fills screen)
 *       └── GtkBox (centered vertically and horizontally)
 *           ├── GtkLabel  (title)
 *           ├── GtkLabel  (body text)
 *           └── GtkButton ("Done")
 *
 * The acknowledgement button is the default focused widget, so keyboard
 * Enter/Space activation follows normal GTK button behavior. Window-manager
 * closure is observed through the window's destroy signal.
 */
static bool show_popup(Popup *popup) {
    GtkWidget *background;
    GtkWidget *box;
    GtkWidget *button;
    GtkWidget *label;
    GtkWidget *label2;
    GtkWidget *overlay;
    GtkCssProvider *provider;

    if (popup_lifecycle_is_open(&popup->lifecycle)) {
        return true;
    }
    if (!popup_lifecycle_open(&popup->lifecycle)) {
        return false;
    }

    // Create an undecorated, always-on-top fullscreen window.
    popup->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (!popup->window) {
        (void)popup_lifecycle_dismiss(
            &popup->lifecycle,
            POPUP_CLOSE_SHUTDOWN
        );
        return false;
    }
    gtk_window_set_title(GTK_WINDOW(popup->window), "Eye Reminder");
    gtk_window_fullscreen(GTK_WINDOW(popup->window));
    gtk_window_set_decorated(GTK_WINDOW(popup->window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(popup->window), TRUE);
    g_signal_connect(
        popup->window,
        "destroy",
        G_CALLBACK(on_popup_destroy),
        popup
    );

    // Overlay allows stacking widgets: background behind, content on top.
    overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(popup->window), overlay);

    /* Drawing area acts as the dark semi-transparent backdrop.
     * It expands to fill the entire screen via hexpand/vexpand. */
    background = gtk_drawing_area_new();
    gtk_widget_set_hexpand(background, TRUE);
    gtk_widget_set_vexpand(background, TRUE);
    gtk_container_add(GTK_CONTAINER(overlay), background);

    // Vertical box centered on screen holds all visible content.
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), box);

    label = gtk_label_new("💧 وقت قطره چشم!");
    label2 = gtk_label_new(
        "یک ساعت از استفاده از صفحه گذشته.\nالان از Artificial Tears استفاده کن."
    );
    button = gtk_button_new_with_label("انجام شد ✅");

    // Connect the button click to our dismiss callback.
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), popup);
    gtk_widget_set_can_default(button, TRUE);
    gtk_window_set_default(GTK_WINDOW(popup->window), button);

    gtk_box_pack_start(GTK_BOX(box), label,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), label2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

    // Apply CSS styling: dark window background, large white text, big button.
    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: rgba(0,0,0,0.85); }\n"
        "label  { font-size: 36px; color: white; }\n"
        "button { font-size: 24px; padding: 10px 20px; }\n",
        -1, NULL
    );
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(provider);

    gtk_widget_show_all(popup->window);
    gtk_widget_grab_focus(button);
    return true;
}

static void shutdown_presentation(Popup *popup) {
    dismiss_popup(popup, POPUP_CLOSE_SHUTDOWN);
    if (notify_is_initted()) {
        notify_uninit();
    }
}

/* print_usage()
 *
 * Prints a short command-line reference to the requested stream.
 */
static void print_usage(FILE *stream, const char *prog) {
    fprintf(stream, "Usage: %s [OPTIONS]\n\n", prog);
    fprintf(stream,
        "  --set-interval <min>   Set interval (%d-%d whole minutes)\n",
        EYEKI_MIN_INTERVAL_MINUTES,
        EYEKI_MAX_INTERVAL_MINUTES);
    fprintf(stream,
        "  --set-mode <mode>      Set mode: n (notification) | p (popup)\n");
    fprintf(stream, "  --show-config          Show current config\n");
    fprintf(stream, "  --version              Show application version\n");
    fprintf(stream, "  --daemon               Run as daemon (default behavior)\n");
    fprintf(stream, "  --help                 Show this help\n");
}

/* main()
 *
 * Entry point. Handles two distinct execution modes:
 *
 * 1. One-shot config commands (--set-interval, --set-mode, --show-config):
 *    Apply the change, persist it via save_config(), then exit immediately.
 *
 * 2. Daemon mode (--daemon or no arguments):
 *    Initialise the notification/GTK subsystem, then enter an infinite loop
 *    that tracks how many seconds the user has been actively using the screen.
 *    Every 10 seconds the loop:
 *      a) Samples a monotonic clock that is unaffected by wall-clock changes.
 *      b) Queries idle time from logind.
 *      c) Adds elapsed time to active_seconds only when idle < IDLE_THRESHOLD_SEC.
 *      d) Fires a reminder (notification or popup) when active_seconds reaches
 *         the configured interval, then resets the counter.
 */
int main(int argc, char *argv[]) {
    Config cfg = load_config();

    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "--set-interval") == 0) {
            int interval_minutes;

            if (i + 1 >= argc) {
                print_usage(stderr, argv[0]);
                return 1;
            }

            i++;
            if (!parse_interval_minutes(argv[i], &interval_minutes)) {
                print_usage(stderr, argv[0]);
                return 1;
            }

            cfg.interval_minutes = interval_minutes;
            if (!save_config(&cfg)) {
                perror("eyeki");
                return 1;
            }
            printf("Interval set to %d minutes.\n", cfg.interval_minutes);
            return 0;

        } else if (strcmp(argv[i], "--set-mode") == 0 && i + 1 < argc) {
            i++;
            // Accept shorthand: "p" = popup, "n" = notification.
            if (strcmp(argv[i], "p") == 0)
                cfg.mode = MODE_POPUP;
            else if (strcmp(argv[i], "n") == 0)
                cfg.mode = MODE_NOTIFICATION;
            else {
                fprintf(stderr, "Invalid mode: %s\n", argv[i]);
                return 1;
            }
            if (!save_config(&cfg)) {
                perror("eyeki");
                return 1;
            }
            if (strcmp(argv[i], "p") == 0) {
                printf("Mode set to: popup\n");
            }
            else {
                printf("Mode set to: notification\n");
            }
            return 0;

        } else if (strcmp(argv[i], "--show-config") == 0) {
            // Dump the current persisted configuration and exit.
            printf("interval=%d\nmode=%s\n",
                cfg.interval_minutes,
                cfg.mode == MODE_POPUP ? "popup" : "notification");
            return 0;

        } else if (strcmp(argv[i], "--version") == 0) {
            printf("EyeKi %s\n", EYEKI_VERSION);
            return 0;

        } else if (strcmp(argv[i], "--daemon") == 0) {
            // Explicit daemon flag — just fall through to the daemon loop.
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(stdout, argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(stderr, argv[0]);
            return 1;
        }
    }

    /* --- Daemon initialisation --- */

    if (!lifecycle_install_signal_handlers()) {
        perror("eyeki");
        return 1;
    }

    ConfigWatch *config_watch = config_watch_create();
    if (!config_watch) {
        perror("eyeki");
        return 1;
    }

    /* Close the load/watch race by loading again after the watch exists. */
    cfg = load_config();

    PopupBackendContext popup_context = {&argc, &argv};
    PresentationBackends presentation;
    Popup popup = { .window = NULL };

    popup_lifecycle_init(&popup.lifecycle);

    if (!presentation_backends_init(
            &presentation,
            initialize_notification_backend,
            NULL,
            initialize_popup_backend,
            &popup_context
        ) || !presentation_backends_prepare(&presentation, cfg.mode)) {
        fprintf(stderr,
            "EyeKi cannot start in the selected presentation mode.\n");
        config_watch_destroy(config_watch);
        return 1;
    }

    fprintf(stderr, "EyeKi started. interval=%dmin mode=%s\n",
        cfg.interval_minutes,
        cfg.mode == MODE_POPUP ? "popup" : "notification");

    RuntimeState runtime;
    struct timespec monotonic_now;
    struct timespec next_activity_sample;
    struct pollfd watched_config = {
        .fd = config_watch_fd(config_watch),
        .events = POLLIN,
        .revents = 0
    };
    ActivityLookupResult last_activity_lookup = ACTIVITY_LOOKUP_OK;

    if (clock_gettime(CLOCK_MONOTONIC, &monotonic_now) < 0 ||
        !runtime_state_init(&runtime, cfg, monotonic_now)) {
        fprintf(stderr, "Failed to initialize reminder scheduler.\n");
        shutdown_presentation(&popup);
        config_watch_destroy(config_watch);
        return 1;
    }
    next_activity_sample = monotonic_now;
    next_activity_sample.tv_sec += ACTIVITY_SAMPLE_INTERVAL_SEC;

    // --- Main daemon loop ---
    while (!lifecycle_shutdown_requested()) {
        PopupCloseReason close_reason;
        int poll_timeout;

        process_popup_events(&popup);
        if (lifecycle_shutdown_requested()) {
            break;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &monotonic_now) < 0) {
            fprintf(stderr, "Failed to read monotonic clock.\n");
            shutdown_presentation(&popup);
            config_watch_destroy(config_watch);
            return 1;
        }

        if (popup_lifecycle_take_close(
                &popup.lifecycle,
                &close_reason
            )) {
            (void)close_reason;
            scheduler_reset(&runtime.scheduler, monotonic_now);
            next_activity_sample = monotonic_now;
            next_activity_sample.tv_sec += ACTIVITY_SAMPLE_INTERVAL_SEC;
        }

        watched_config.revents = 0;
        poll_timeout = milliseconds_until(
            next_activity_sample,
            monotonic_now
        );
        if (popup_lifecycle_is_open(&popup.lifecycle) &&
            poll_timeout > POPUP_EVENT_INTERVAL_MILLISECONDS) {
            poll_timeout = POPUP_EVENT_INTERVAL_MILLISECONDS;
        }
        int poll_result = poll(
            &watched_config,
            1,
            poll_timeout
        );
        if (poll_result < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("eyeki");
            shutdown_presentation(&popup);
            config_watch_destroy(config_watch);
            return 1;
        }

        if (lifecycle_shutdown_requested()) {
            break;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &monotonic_now) < 0) {
            fprintf(stderr, "Failed to read monotonic clock.\n");
            shutdown_presentation(&popup);
            config_watch_destroy(config_watch);
            return 1;
        }

        if (watched_config.revents & POLLIN) {
            if (!reload_config(
                    config_watch,
                    &runtime,
                    &presentation,
                    monotonic_now
                )) {
                perror("eyeki");
                shutdown_presentation(&popup);
                config_watch_destroy(config_watch);
                return 1;
            }
            if (runtime.config.mode != MODE_POPUP) {
                dismiss_popup(&popup, POPUP_CLOSE_MODE_CHANGE);
            }
        }

        if (watched_config.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            errno = EIO;
            perror("eyeki");
            shutdown_presentation(&popup);
            config_watch_destroy(config_watch);
            return 1;
        }

        if (!timespec_at_or_after(monotonic_now, next_activity_sample)) {
            continue;
        }

        /* A visible popup pauses active-time counting until dismissal. */
        if (popup_lifecycle_is_open(&popup.lifecycle)) {
            scheduler_reset(&runtime.scheduler, monotonic_now);
            next_activity_sample = monotonic_now;
            next_activity_sample.tv_sec += ACTIVITY_SAMPLE_INTERVAL_SEC;
            continue;
        }

        int idle_seconds = 0;
        ActivityState activity;
        ActivityLookupResult activity_lookup =
            activity_get_idle_seconds(&idle_seconds);
        if (activity_lookup != ACTIVITY_LOOKUP_OK) {
            activity = ACTIVITY_UNKNOWN;
            if (activity_lookup != last_activity_lookup) {
                if (activity_lookup == ACTIVITY_LOOKUP_NO_SESSION) {
                    fprintf(stderr,
                        "No active local graphical session for this user; "
                        "timer reset.\n");
                } else if (activity_lookup == ACTIVITY_LOOKUP_AMBIGUOUS) {
                    fprintf(stderr,
                        "Multiple active graphical sessions are ambiguous; "
                        "timer reset.\n");
                } else {
                    fprintf(stderr,
                        "Unable to query the current session's idle state; "
                        "timer reset.\n");
                }
            }
        } else {
            activity = idle_seconds >= IDLE_THRESHOLD_SEC
                ? ACTIVITY_IDLE
                : ACTIVITY_ACTIVE;
            if (last_activity_lookup != ACTIVITY_LOOKUP_OK) {
                fprintf(stderr, "Idle-state lookup recovered.\n");
            }
        }
        last_activity_lookup = activity_lookup;

        /* Idle and unknown samples both discard all accumulated active time. */
        if (scheduler_record_sample(
                &runtime.scheduler,
                activity,
                monotonic_now
            )) {
            if (runtime.config.mode == MODE_POPUP) {
                if (!show_popup(&popup)) {
                    fprintf(stderr, "Failed to create reminder popup.\n");
                    shutdown_presentation(&popup);
                    config_watch_destroy(config_watch);
                    return 1;
                }
            } else {
                (void)send_notification();
            }

            if (clock_gettime(CLOCK_MONOTONIC, &monotonic_now) < 0) {
                fprintf(stderr, "Failed to restart reminder scheduler.\n");
                shutdown_presentation(&popup);
                config_watch_destroy(config_watch);
                return 1;
            }
            scheduler_reset(&runtime.scheduler, monotonic_now);
        }

        next_activity_sample = monotonic_now;
        next_activity_sample.tv_sec += ACTIVITY_SAMPLE_INTERVAL_SEC;
    }

    shutdown_presentation(&popup);
    config_watch_destroy(config_watch);
    return 0;
}
