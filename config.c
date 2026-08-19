#include "config.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SECONDS_PER_MINUTE 60ULL

Config default_config(void) {
    Config cfg;
    cfg.interval_minutes = 60;
    cfg.mode = MODE_POPUP;
    return cfg;
}

bool parse_interval_minutes(const char *value, int *interval_minutes) {
    char *end;
    long parsed;
    const char *cursor;

    if (!value || !interval_minutes || value[0] == '\0') {
        return false;
    }

    for (cursor = value; *cursor != '\0'; cursor++) {
        if (*cursor < '0' || *cursor > '9') {
            return false;
        }
    }

    errno = 0;
    parsed = strtol(value, &end, 10);
    if (errno == ERANGE || *end != '\0' ||
        parsed < EYEKI_MIN_INTERVAL_MINUTES ||
        parsed > EYEKI_MAX_INTERVAL_MINUTES) {
        return false;
    }

    *interval_minutes = (int)parsed;
    return true;
}

bool interval_minutes_to_seconds(int interval_minutes, uint64_t *seconds) {
    uint64_t minutes;

    if (!seconds || interval_minutes < EYEKI_MIN_INTERVAL_MINUTES ||
        interval_minutes > EYEKI_MAX_INTERVAL_MINUTES) {
        return false;
    }

    minutes = (uint64_t)interval_minutes;
    if (minutes > UINT64_MAX / SECONDS_PER_MINUTE) {
        return false;
    }

    *seconds = minutes * SECONDS_PER_MINUTE;
    return true;
}

Config load_config(void) {
    Config cfg = default_config();
    const char *home = getenv("HOME");
    char path[PATH_MAX];
    FILE *file;
    char line[256];
    int parsed_interval;
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
            if (parse_interval_minutes(line + 9, &parsed_interval)) {
                cfg.interval_minutes = parsed_interval;
            }
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
