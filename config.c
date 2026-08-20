#define _POSIX_C_SOURCE 200809L

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SECONDS_PER_MINUTE 60ULL
#define CONFIG_DIRECTORY_NAME "eye_reminder"
#define CONFIG_FILE_NAME "config"
#define TEMP_FILE_ATTEMPTS 100

static bool build_config_path(
    char *path,
    size_t path_size,
    bool legacy,
    bool *uses_xdg
) {
    const char *home = getenv("HOME");
    const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
    const char *base;
    int result;

    if (!path || path_size == 0) {
        errno = EINVAL;
        return false;
    }

    if (!legacy && xdg_config_home && xdg_config_home[0] == '/') {
        base = xdg_config_home;
        if (uses_xdg) {
            *uses_xdg = true;
        }
        result = snprintf(
            path,
            path_size,
            "%s/%s/%s",
            base,
            CONFIG_DIRECTORY_NAME,
            CONFIG_FILE_NAME
        );
    } else {
        if (!home || home[0] != '/') {
            errno = ENOENT;
            return false;
        }
        if (uses_xdg) {
            *uses_xdg = false;
        }
        result = snprintf(
            path,
            path_size,
            "%s/.config/%s/%s",
            home,
            CONFIG_DIRECTORY_NAME,
            CONFIG_FILE_NAME
        );
    }

    if (result < 0 || (size_t)result >= path_size) {
        errno = ENAMETOOLONG;
        return false;
    }

    return true;
}

static bool build_config_directory(char *directory, size_t directory_size) {
    const char *home = getenv("HOME");
    const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
    const char *base;
    int result;

    if (!directory || directory_size == 0) {
        errno = EINVAL;
        return false;
    }

    if (xdg_config_home && xdg_config_home[0] == '/') {
        base = xdg_config_home;
        result = snprintf(
            directory,
            directory_size,
            "%s/%s",
            base,
            CONFIG_DIRECTORY_NAME
        );
    } else {
        if (!home || home[0] != '/') {
            errno = ENOENT;
            return false;
        }
        result = snprintf(
            directory,
            directory_size,
            "%s/.config/%s",
            home,
            CONFIG_DIRECTORY_NAME
        );
    }

    if (result < 0 || (size_t)result >= directory_size) {
        errno = ENAMETOOLONG;
        return false;
    }

    return true;
}

static bool load_config_file(const char *path, Config *cfg) {
    Config parsed = *cfg;
    int fd;
    FILE *file;
    char line[256];
    int parsed_interval;
    int saved_errno;

    fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        return false;
    }

    file = fdopen(fd, "r");
    if (!file) {
        saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return false;
    }

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "interval=", 9) == 0) {
            if (parse_interval_minutes(line + 9, &parsed_interval)) {
                parsed.interval_minutes = parsed_interval;
            }
        } else if (strncmp(line, "mode=", 5) == 0) {
            parsed.mode = strcmp(line + 5, "popup") == 0
                ? MODE_POPUP
                : MODE_NOTIFICATION;
        }
    }

    if (ferror(file)) {
        saved_errno = errno ? errno : EIO;
        fclose(file);
        errno = saved_errno;
        return false;
    }
    if (fclose(file) != 0) {
        return false;
    }

    *cfg = parsed;
    return true;
}

static int open_config_directory(const char *directory) {
    char path[PATH_MAX];
    char *cursor;
    int directory_fd;

    if (!directory || directory[0] != '/') {
        errno = EINVAL;
        return -1;
    }
    if (strlen(directory) >= sizeof(path)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(path, directory);

    directory_fd = open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory_fd < 0) {
        return -1;
    }

    cursor = path + 1;
    while (*cursor != '\0') {
        char *separator;
        char *next;
        bool final_component;
        bool created = false;
        int child_fd;

        while (*cursor == '/') {
            cursor++;
        }
        if (*cursor == '\0') {
            break;
        }

        separator = strchr(cursor, '/');
        if (separator) {
            *separator = '\0';
            next = separator + 1;
            while (*next == '/') {
                next++;
            }
        } else {
            next = cursor + strlen(cursor);
        }
        final_component = *next == '\0';

        child_fd = openat(
            directory_fd,
            cursor,
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW
        );
        if (child_fd < 0 && errno == ENOENT) {
            mode_t previous_umask = umask(0);
            int mkdir_result = mkdirat(directory_fd, cursor, 0700);
            int mkdir_errno = errno;

            umask(previous_umask);
            if (mkdir_result == 0) {
                created = true;
            } else if (mkdir_errno != EEXIST) {
                close(directory_fd);
                errno = mkdir_errno;
                return -1;
            }
            child_fd = openat(
                directory_fd,
                cursor,
                O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW
            );
        }
        if (child_fd < 0) {
            int saved_errno = errno;
            close(directory_fd);
            errno = saved_errno;
            return -1;
        }

        if ((created || final_component) && fchmod(child_fd, 0700) < 0) {
            int saved_errno = errno;
            close(child_fd);
            close(directory_fd);
            errno = saved_errno;
            return -1;
        }

        close(directory_fd);
        directory_fd = child_fd;
        cursor = next;
    }

    return directory_fd;
}

static int create_temporary_file(int directory_fd, char *name, size_t name_size) {
    unsigned int attempt;

    for (attempt = 0; attempt < TEMP_FILE_ATTEMPTS; attempt++) {
        int result = snprintf(
            name,
            name_size,
            ".config.tmp.%ld.%u",
            (long)getpid(),
            attempt
        );
        int fd;

        if (result < 0 || (size_t)result >= name_size) {
            errno = ENAMETOOLONG;
            return -1;
        }

        fd = openat(
            directory_fd,
            name,
            O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
            0600
        );
        if (fd >= 0) {
            if (fchmod(fd, 0600) < 0) {
                int saved_errno = errno;
                close(fd);
                unlinkat(directory_fd, name, 0);
                errno = saved_errno;
                return -1;
            }
            return fd;
        }
        if (errno != EEXIST) {
            return -1;
        }
    }

    errno = EEXIST;
    return -1;
}

static bool write_config_atomic(int directory_fd, const Config *cfg) {
    char temporary_name[64];
    int fd;
    FILE *file;
    int saved_errno;

    fd = create_temporary_file(
        directory_fd,
        temporary_name,
        sizeof(temporary_name)
    );
    if (fd < 0) {
        return false;
    }

    file = fdopen(fd, "w");
    if (!file) {
        saved_errno = errno;
        close(fd);
        unlinkat(directory_fd, temporary_name, 0);
        errno = saved_errno;
        return false;
    }

    if (fprintf(file, "interval=%d\n", cfg->interval_minutes) < 0 ||
        fprintf(
            file,
            "mode=%s\n",
            cfg->mode == MODE_POPUP ? "popup" : "notification"
        ) < 0 ||
        fflush(file) != 0 || fsync(fileno(file)) != 0) {
        saved_errno = errno ? errno : EIO;
        fclose(file);
        unlinkat(directory_fd, temporary_name, 0);
        errno = saved_errno;
        return false;
    }
    if (fclose(file) != 0) {
        saved_errno = errno;
        unlinkat(directory_fd, temporary_name, 0);
        errno = saved_errno;
        return false;
    }

    if (renameat(
            directory_fd,
            temporary_name,
            directory_fd,
            CONFIG_FILE_NAME
        ) < 0) {
        saved_errno = errno;
        unlinkat(directory_fd, temporary_name, 0);
        errno = saved_errno;
        return false;
    }

    if (fsync(directory_fd) < 0) {
        return false;
    }

    return true;
}

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
    char path[PATH_MAX];
    char legacy_path[PATH_MAX];
    bool uses_xdg = false;
    int load_errno;

    if (!build_config_path(path, sizeof(path), false, &uses_xdg)) {
        return cfg;
    }
    if (load_config_file(path, &cfg)) {
        return cfg;
    }
    load_errno = errno;

    if (uses_xdg && load_errno == ENOENT &&
        build_config_path(legacy_path, sizeof(legacy_path), true, NULL) &&
        strcmp(path, legacy_path) != 0) {
        load_config_file(legacy_path, &cfg);
    }

    return cfg;
}

bool save_config(const Config *cfg) {
    char directory[PATH_MAX];
    int directory_fd;
    bool saved;
    int saved_errno;

    if (!cfg) {
        errno = EINVAL;
        return false;
    }
    if (!build_config_directory(directory, sizeof(directory))) {
        return false;
    }

    directory_fd = open_config_directory(directory);
    if (directory_fd < 0) {
        return false;
    }

    saved = write_config_atomic(directory_fd, cfg);
    saved_errno = errno;
    close(directory_fd);
    errno = saved_errno;
    return saved;
}
