/* config.h
 *
 * Configuration management for EyeKi.
 *
 * This header defines the data types and helper functions used to manage
 * persistent user settings. All three functions (default_config, load_config,
 * save_config) are implemented directly in this header — intentionally, since
 * the project is a single translation unit. Including this header in more than
 * one .c file would cause duplicate-definition linker errors.
 *
 * Config file location (created automatically if absent):
 *   $HOME/.config/eye_reminder/config
 *
 * File format (plain text, one key=value per line):
 *   interval=60
 *   mode=notification
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   /* mkdir()    */
#include <sys/types.h>  /* mode_t     */
#include <limits.h>     /* PATH_MAX   */

/* ---------------------------------------------------------------------------
 * ReminderMode
 *
 * Selects how the user is notified when the active-screen timer fires.
 *
 *   MODE_NOTIFICATION  – A standard desktop notification via libnotify.
 *                        Non-intrusive; disappears automatically.
 *
 *   MODE_POPUP         – A fullscreen GTK overlay window that demands
 *                        explicit acknowledgement before it closes.
 * ---------------------------------------------------------------------------
 */
typedef enum {
    MODE_NOTIFICATION,
    MODE_POPUP
} ReminderMode;

/* ---------------------------------------------------------------------------
 * Config
 *
 * Holds all user-configurable settings in one flat struct so they can be
 * passed around, saved, and loaded as a single unit.
 *
 * Fields:
 *   interval_minutes  – How many minutes of continuous active screen time
 *                       must pass before a reminder is triggered.
 *   mode              – Which notification style to use (see ReminderMode).
 * ---------------------------------------------------------------------------
 */
typedef struct {
    int          interval_minutes;
    ReminderMode mode;
} Config;

/* ---------------------------------------------------------------------------
 * default_config()
 *
 * Returns a Config struct pre-filled with safe, conservative defaults.
 * These values are used:
 *   - The very first time the program runs (no config file exists yet).
 *   - As a fallback when the config file cannot be opened or parsed.
 *
 * Defaults:
 *   interval_minutes = 60
 *   mode             = MODE_POPUP
 * ---------------------------------------------------------------------------
 */
Config default_config() {
    Config cfg;
    cfg.interval_minutes = 60;
    cfg.mode = MODE_POPUP;
    return cfg;
}

/* ---------------------------------------------------------------------------
 * load_config()
 *
 * Reads settings from the persistent config file and returns a populated
 * Config struct. Falls back to default_config() at any failure point so
 * the caller always receives a valid, usable Config.
 *
 * Steps:
 *   1. Retrieve $HOME from the environment; bail if unset.
 *   2. Build the full file path safely with snprintf + PATH_MAX bounds check.
 *   3. Open the file for reading; bail (with defaults) if missing.
 *   4. Parse each line:
 *        "interval=<N>"  → cfg.interval_minutes = N   (atoi conversion)
 *        "mode=popup"    → cfg.mode = MODE_POPUP
 *        "mode=<other>"  → cfg.mode = MODE_NOTIFICATION
 *      Unknown keys are silently ignored, making the format forward-compatible.
 *   5. Strip the trailing newline with strcspn before comparing keys.
 *
 * Returns:
 *   A Config struct with values from the file, or defaults on any error.
 * ---------------------------------------------------------------------------
 */
Config load_config() {
    Config cfg = default_config();

    char *home = getenv("HOME");
    if (!home) return cfg;

    char path[PATH_MAX];
    int ret = snprintf(path, sizeof(path), "%s/.config/eye_reminder/config", home);
    /* snprintf returns the number of characters that *would* have been written.
     * If ret >= sizeof(path), the path was truncated — treat as error. */
    if (ret < 0 || ret >= (int)sizeof(path)) return cfg;

    FILE *f = fopen(path, "r");
    if (!f) return cfg;  /* File absent on first run — defaults are fine. */

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        /* Remove the trailing newline so strcmp/strncmp work correctly. */
        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "interval=", 9) == 0) {
            /* Parse the integer value that follows "interval=". */
            cfg.interval_minutes = atoi(line + 9);

        } else if (strncmp(line, "mode=", 5) == 0) {
            /* Only "popup" maps to MODE_POPUP; everything else (including
             * "notification") maps to MODE_NOTIFICATION. */
            if (strcmp(line + 5, "popup") == 0) {
                cfg.mode = MODE_POPUP;
            } else {
                cfg.mode = MODE_NOTIFICATION;
            }
        }
        /* Unrecognised keys are intentionally skipped here. */
    }

    fclose(f);
    return cfg;
}

/* ---------------------------------------------------------------------------
 * save_config()
 *
 * Persists the current Config struct to disk so settings survive restarts.
 *
 * Steps:
 *   1. Retrieve $HOME; bail silently if unset.
 *   2. Build the directory path and full file path with bounds-checked snprintf.
 *   3. Create the directory with mkdir(dir, 0755).
 *      - 0755 → owner has full access; group/others can read and traverse.
 *      - If the directory already exists, mkdir() returns EEXIST which is
 *        harmless — the error is intentionally not checked here.
 *   4. Open (or truncate) the config file for writing.
 *   5. Write each setting as "key=value\n".
 *      - interval is written as a decimal integer.
 *      - mode is written as the human-readable string "popup" or "notification".
 *
 * Parameters:
 *   cfg  – Pointer to the Config to be written (not modified).
 * ---------------------------------------------------------------------------
 */
void save_config(Config *cfg) {
    char *home = getenv("HOME");
    if (!home) return;

    char dir[PATH_MAX];
    char path[PATH_MAX];

    /* Build the directory path: $HOME/.config/eye_reminder */
    int ret = snprintf(dir, sizeof(dir), "%s/.config/eye_reminder", home);
    if (ret < 0 || ret >= (int)sizeof(dir)) return;

    /* Build the full file path: <dir>/config */
    ret = snprintf(path, sizeof(path), "%s/config", dir);
    if (ret < 0 || ret >= (int)sizeof(path)) return;

    /* Ensure the directory exists. 0755 grants rwx to owner, r-x to others.
     * EEXIST (directory already there) is silently accepted. */
    mkdir(dir, 0755);

    /* Open for writing; creates the file if absent, truncates if present. */
    FILE *f = fopen(path, "w");
    if (!f) return;

    fprintf(f, "interval=%d\n", cfg->interval_minutes);
    fprintf(f, "mode=%s\n", cfg->mode == MODE_POPUP ? "popup" : "notification");

    fclose(f);
}

#endif /* CONFIG_H */
