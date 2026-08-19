#include "config.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

Config default_config(void) {
    Config cfg;
    cfg.interval_minutes = 60;
    cfg.mode = MODE_POPUP;
    return cfg;
}

Config load_config(void) {
    Config cfg = default_config();
    const char *home = getenv("HOME");
    char path[PATH_MAX];
    FILE *file;
    char line[256];
    int result;

    if (!home) {
        return cfg;
    }

    result = snprintf(path, sizeof(path), "%s/.config/eye_reminder/config", home);
    if (result < 0 || result >= (int)sizeof(path)) {
        return cfg;
    }

    file = fopen(path, "r");
    if (!file) {
        return cfg;
    }

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "interval=", 9) == 0) {
            cfg.interval_minutes = atoi(line + 9);
        } else if (strncmp(line, "mode=", 5) == 0) {
            cfg.mode = strcmp(line + 5, "popup") == 0
                ? MODE_POPUP
                : MODE_NOTIFICATION;
        }
    }

    fclose(file);
    return cfg;
}

void save_config(const Config *cfg) {
    const char *home = getenv("HOME");
    char directory[PATH_MAX];
    char path[PATH_MAX];
    FILE *file;
    int result;

    if (!home) {
        return;
    }

    result = snprintf(
        directory,
        sizeof(directory),
        "%s/.config/eye_reminder",
        home
    );
    if (result < 0 || result >= (int)sizeof(directory)) {
        return;
    }

    result = snprintf(path, sizeof(path), "%s/config", directory);
    if (result < 0 || result >= (int)sizeof(path)) {
        return;
    }

    mkdir(directory, 0755);

    file = fopen(path, "w");
    if (!file) {
        return;
    }

    fprintf(file, "interval=%d\n", cfg->interval_minutes);
    fprintf(
        file,
        "mode=%s\n",
        cfg->mode == MODE_POPUP ? "popup" : "notification"
    );

    fclose(file);
}
