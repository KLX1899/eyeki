#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <systemd/sd-bus.h>
#include <libnotify/notify.h>
#include <gtk/gtk.h>
#include "config.h"
#include "scheduler.h"
#include "version.h"

/* If the user has been idle for this long (in seconds), discard accumulated
   active time and require a fresh continuous-use interval. */
#define IDLE_THRESHOLD_SEC 60

// Global reference to the popup window so the button callback can destroy it.
static GtkWidget *popup_window = NULL;

// Flag set to 1 when the user clicks "Done" to break the popup event loop.
static int popup_dismissed = 0;

/* get_idle_seconds()
 *
 * Queries systemd-logind over D-Bus to determine how long the current
 * session has been idle.
 *
 * Strategy:
 *   1. Connect to the system bus.
 *   2. Call ListSessions on the logind Manager to get the first session path.
 *   3. Read the boolean property IdleHint on that session.
 *      - If false  → user is active, return 0 immediately.
 *      - If true   → user is idle, continue to step 4.
 *   4. Read IdleSinceHintMonotonic (a microsecond-precision monotonic timestamp
 *      of when the session became idle).
 *   5. Subtract that timestamp from the current time to get elapsed idle
 *      seconds and return it.
 *
 * Returns true and writes the idle duration on success. Returns false when
 * logind cannot provide a trustworthy result, so callers do not confuse an
 * unknown state with user activity.
 */
bool get_idle_seconds(int *idle_seconds) {
    sd_bus *bus = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    int ret;

    // Open a connection to the system D-Bus (not the session bus).
    if (!idle_seconds) return false;
    *idle_seconds = 0;

    ret = sd_bus_open_system(&bus);
    if (ret < 0) return false;

    /* Ask logind for the list of all active login sessions.
     * The reply is an array of (susso) structs:
     *   s = session id, u = uid, s = username, s = seat, o = object path */
    ret = sd_bus_call_method(
        bus,
        "org.freedesktop.login1",
        "/org/freedesktop/login1",
        "org.freedesktop.login1.Manager",
        "ListSessions",
        &error, &reply, ""
    );
    if (ret < 0) goto cleanup;

    // Enter the array container so we can read individual session entries.
    ret = sd_bus_message_enter_container(reply, 'a', "(susso)");
    if (ret < 0) goto cleanup;

    {
        const char *session_id, *user_name, *seat_id, *session_path;
        uint32_t uid;

        /* Read only the first session entry.
         * For a typical single-user desktop this is sufficient.
         * session_path is the D-Bus object path we need for further queries. */
        ret = sd_bus_message_read(reply, "(susso)",
            &session_id, &uid, &user_name, &seat_id, &session_path);
        if (ret < 0) goto cleanup;

        /* We are done with the ListSessions reply; release it before making
         * new property calls on the session object. */
        sd_bus_message_unref(reply);
        reply = NULL;
        sd_bus_error_free(&error);
        error = SD_BUS_ERROR_NULL;

        /* Read the IdleHint boolean property from the session object.
         * IdleHint is set to true by logind when no input has been received
         * for the configured IdleAction timeout. */
        int idle_hint = 0;
        ret = sd_bus_get_property_trivial(
            bus,
            "org.freedesktop.login1",
            session_path,
            "org.freedesktop.login1.Session",
            "IdleHint",
            &error,
            'b',          /* D-Bus type: boolean */
            &idle_hint
        );
        if (ret < 0) goto cleanup;

        // User is actively using the session — no idle time to report.
        if (!idle_hint) {
            sd_bus_unref(bus);
            return true;
        }

        /* Session is idle. Now find out exactly when it became idle so we
         * can compute the elapsed duration. */
        uint64_t idle_since = 0;
        sd_bus_error_free(&error);
        error = SD_BUS_ERROR_NULL;

        /* IdleSinceHintMonotonic is a uint64 microsecond monotonic timestamp
         * representing the moment the session transitioned to idle. */
        ret = sd_bus_get_property_trivial(
            bus,
            "org.freedesktop.login1",
            session_path,
            "org.freedesktop.login1.Session",
            "IdleSinceHintMonotonic",
            &error,
            't',          /* D-Bus type: uint64 */
            &idle_since
        );
        if (ret < 0 || idle_since == 0) goto cleanup;

        /* Use the same monotonic clock domain as IdleSinceHintMonotonic. */
        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) goto cleanup;
        uint64_t now_usec = (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;

        if (idle_since > now_usec) goto cleanup;

        // Convert the difference from microseconds to whole seconds.
        uint64_t idle_elapsed = (now_usec - idle_since) / 1000000ULL;
        *idle_seconds = idle_elapsed > INT_MAX ? INT_MAX : (int)idle_elapsed;
        sd_bus_unref(bus);
        return true;
    }

cleanup:
    // Reached on any D-Bus error; free all resources and report "not idle".
    if (reply) sd_bus_message_unref(reply);
    sd_bus_error_free(&error);
    sd_bus_unref(bus);
    return false;
}

/* send_notification()
 *
 * Sends a desktop notification via libnotify.
 * notify_init() is called lazily — only on the first invocation.
 * The notification auto-dismisses after 10 seconds (10000 ms).
 */
void send_notification() {
    if (!notify_is_initted()) {
        notify_init("EyeKi");
    }
    NotifyNotification *n = notify_notification_new(
        "💧 وقت قطره چشم!",
        "یک ساعت از روشن بودن صفحه گذشته.\nالان از Artificial Tears استفاده کن.",
        "dialog-information"
    );
    notify_notification_set_urgency(n, NOTIFY_URGENCY_NORMAL);
    notify_notification_set_timeout(n, 10000); /* milliseconds */
    notify_notification_show(n, NULL);
    g_object_unref(G_OBJECT(n));
}

/* on_button_clicked()
 *
 * GTK signal callback connected to the "Done" button inside the popup.
 * Sets the global flag and destroys the window so show_popup()'s event
 * loop can exit cleanly.
 */
static void on_button_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data; /* suppress unused-parameter warnings */
    popup_dismissed = 1;
    if (popup_window) {
        gtk_widget_destroy(popup_window);
        popup_window = NULL;
    }
}

/* show_popup()
 *
 * Builds and displays a fullscreen GTK overlay window that blocks the
 * user's view until they confirm they have used their eye drops.
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
 * The function runs a manual GTK event loop (instead of gtk_main()) so
 * that control returns to the daemon loop as soon as the popup is dismissed.
 */
void show_popup() {
    popup_dismissed = 0;

    // Create an undecorated, always-on-top fullscreen window.
    popup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(popup_window), "Eye Reminder");
    gtk_window_fullscreen(GTK_WINDOW(popup_window));
    gtk_window_set_decorated(GTK_WINDOW(popup_window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(popup_window), TRUE);

    // Overlay allows stacking widgets: background behind, content on top.
    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(popup_window), overlay);

    /* Drawing area acts as the dark semi-transparent backdrop.
     * It expands to fill the entire screen via hexpand/vexpand. */
    GtkWidget *background = gtk_drawing_area_new();
    gtk_widget_set_hexpand(background, TRUE);
    gtk_widget_set_vexpand(background, TRUE);
    gtk_container_add(GTK_CONTAINER(overlay), background);

    // Vertical box centered on screen holds all visible content.
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), box);

    GtkWidget *label  = gtk_label_new("💧 وقت قطره چشم!");
    GtkWidget *label2 = gtk_label_new(
        "یک ساعت از استفاده از صفحه گذشته.\nالان از Artificial Tears استفاده کن."
    );
    GtkWidget *button = gtk_button_new_with_label("انجام شد ✅");

    // Connect the button click to our dismiss callback.
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(box), label,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), label2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

    // Apply CSS styling: dark window background, large white text, big button.
    GtkCssProvider *provider = gtk_css_provider_new();
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

    gtk_widget_show_all(popup_window);

    /* Manual event loop: process GTK events until the user clicks "Done".
     * usleep(50000) = 50 ms between iterations to avoid busy-waiting. */
    while (!popup_dismissed) {
        while (gtk_events_pending())
            gtk_main_iteration();
        usleep(50000);
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

    /* GTK must be initialised before any window is created.
     * libnotify does not need GTK, so we only init what we actually need. */
    if (cfg.mode == MODE_POPUP) {
        gtk_init(&argc, &argv);
    } else {
        notify_init("EyeKi");
    }

    fprintf(stderr, "EyeKi started. interval=%dmin mode=%s\n",
        cfg.interval_minutes,
        cfg.mode == MODE_POPUP ? "popup" : "notification");

    Scheduler scheduler;
    struct timespec monotonic_now;
    uint64_t interval_seconds;
    bool activity_error_reported = false;

    if (!interval_minutes_to_seconds(
            cfg.interval_minutes,
            &interval_seconds
        ) ||
        clock_gettime(CLOCK_MONOTONIC, &monotonic_now) < 0 ||
        !scheduler_init(
            &scheduler,
            interval_seconds,
            monotonic_now
        )) {
        fprintf(stderr, "Failed to initialize reminder scheduler.\n");
        return 1;
    }

    // --- Main daemon loop ---
    while (1) {
        /* Sleep between checks to avoid wasting CPU.
         * 10 s gives ~6 checks per minute which is precise enough. */
        sleep(10);

        if (clock_gettime(CLOCK_MONOTONIC, &monotonic_now) < 0) {
            fprintf(stderr, "Failed to read monotonic clock.\n");
            return 1;
        }

        int idle_seconds = 0;
        ActivityState activity;
        if (!get_idle_seconds(&idle_seconds)) {
            activity = ACTIVITY_UNKNOWN;
            if (!activity_error_reported) {
                fprintf(stderr, "Unable to determine idle state; timer reset.\n");
                activity_error_reported = true;
            }
        } else {
            activity = idle_seconds >= IDLE_THRESHOLD_SEC
                ? ACTIVITY_IDLE
                : ACTIVITY_ACTIVE;
            if (activity_error_reported) {
                fprintf(stderr, "Idle-state lookup recovered.\n");
                activity_error_reported = false;
            }
        }

        /* Idle and unknown samples both discard all accumulated active time. */
        if (scheduler_record_sample(&scheduler, activity, monotonic_now)) {
            // Reload config to pick up any changes made via --set-mode or --set-interval
            cfg = load_config();

            if (cfg.mode == MODE_POPUP)
                show_popup();
            else
                send_notification();

            if (!interval_minutes_to_seconds(
                    cfg.interval_minutes,
                    &interval_seconds
                ) ||
                clock_gettime(CLOCK_MONOTONIC, &monotonic_now) < 0 ||
                !scheduler_init(
                    &scheduler,
                    interval_seconds,
                    monotonic_now
                )) {
                fprintf(stderr, "Failed to restart reminder scheduler.\n");
                return 1;
            }
        }

    }

    return 0;
}
