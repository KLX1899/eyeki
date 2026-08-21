#define _POSIX_C_SOURCE 200809L

#include "config_watch.h"

#include "config.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

#define CONFIG_WATCH_MASK \
    (IN_ATTRIB | IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_DELETE_SELF | \
     IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF)

typedef struct {
    bool exists;
    dev_t device;
    ino_t inode;
    off_t size;
    mode_t mode;
    struct timespec modified;
    struct timespec changed;
} ConfigFileState;

struct ConfigWatch {
    int inotify_fd;
    int watch_descriptor;
    char config_path[PATH_MAX];
    char config_directory[PATH_MAX];
    ConfigFileState file_state;
};

static bool config_file_state(
    const char *path,
    ConfigFileState *file_state
) {
    struct stat status;

    memset(file_state, 0, sizeof(*file_state));
    if (lstat(path, &status) == 0) {
        file_state->exists = true;
        file_state->device = status.st_dev;
        file_state->inode = status.st_ino;
        file_state->size = status.st_size;
        file_state->mode = status.st_mode;
        file_state->modified = status.st_mtim;
        file_state->changed = status.st_ctim;
        return true;
    }

    if (errno == ENOENT || errno == ENOTDIR) {
        return true;
    }
    return false;
}

static bool config_file_state_equal(
    const ConfigFileState *left,
    const ConfigFileState *right
) {
    if (left->exists != right->exists) {
        return false;
    }
    if (!left->exists) {
        return true;
    }

    return left->device == right->device &&
        left->inode == right->inode &&
        left->size == right->size &&
        left->mode == right->mode &&
        left->modified.tv_sec == right->modified.tv_sec &&
        left->modified.tv_nsec == right->modified.tv_nsec &&
        left->changed.tv_sec == right->changed.tv_sec &&
        left->changed.tv_nsec == right->changed.tv_nsec;
}

static bool set_config_directory(ConfigWatch *watch) {
    char *separator;
    size_t length;

    length = strlen(watch->config_path);
    if (length >= sizeof(watch->config_directory)) {
        errno = ENAMETOOLONG;
        return false;
    }
    memcpy(watch->config_directory, watch->config_path, length + 1);

    separator = strrchr(watch->config_directory, '/');
    if (!separator) {
        errno = EINVAL;
        return false;
    }
    if (separator == watch->config_directory) {
        separator[1] = '\0';
    } else {
        *separator = '\0';
    }
    return true;
}

static bool parent_directory(char *path) {
    char *separator;

    if (strcmp(path, "/") == 0) {
        errno = ENOENT;
        return false;
    }

    separator = strrchr(path, '/');
    if (!separator) {
        errno = EINVAL;
        return false;
    }
    if (separator == path) {
        path[1] = '\0';
    } else {
        *separator = '\0';
    }
    return true;
}

static bool refresh_directory_watch(ConfigWatch *watch) {
    char candidate[PATH_MAX];
    int descriptor;

    memcpy(
        candidate,
        watch->config_directory,
        strlen(watch->config_directory) + 1
    );

    while (true) {
        descriptor = inotify_add_watch(
            watch->inotify_fd,
            candidate,
            CONFIG_WATCH_MASK | IN_ONLYDIR | IN_DONT_FOLLOW
        );
        if (descriptor >= 0) {
            break;
        }
        if (errno != ENOENT && errno != ENOTDIR) {
            return false;
        }
        if (!parent_directory(candidate)) {
            return false;
        }
    }

    if (watch->watch_descriptor >= 0 &&
        watch->watch_descriptor != descriptor) {
        if (inotify_rm_watch(
                watch->inotify_fd,
                watch->watch_descriptor
            ) < 0 && errno != EINVAL) {
            return false;
        }
    }
    watch->watch_descriptor = descriptor;
    return true;
}

ConfigWatch *config_watch_create(void) {
    ConfigWatch *watch = calloc(1, sizeof(*watch));
    int saved_errno;

    if (!watch) {
        return NULL;
    }
    watch->inotify_fd = -1;
    watch->watch_descriptor = -1;

    if (!config_get_path(watch->config_path, sizeof(watch->config_path)) ||
        !set_config_directory(watch)) {
        saved_errno = errno;
        free(watch);
        errno = saved_errno;
        return NULL;
    }

    watch->inotify_fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
    if (watch->inotify_fd < 0 || !refresh_directory_watch(watch) ||
        !config_file_state(watch->config_path, &watch->file_state)) {
        saved_errno = errno;
        config_watch_destroy(watch);
        errno = saved_errno;
        return NULL;
    }

    return watch;
}

int config_watch_fd(const ConfigWatch *watch) {
    if (!watch) {
        errno = EINVAL;
        return -1;
    }
    return watch->inotify_fd;
}

bool config_watch_process(ConfigWatch *watch, bool *changed) {
    union {
        char bytes[4096];
        struct inotify_event alignment;
    } buffer;
    ConfigFileState current_state;

    if (!watch || !changed) {
        errno = EINVAL;
        return false;
    }
    *changed = false;

    while (true) {
        ssize_t length = read(
            watch->inotify_fd,
            buffer.bytes,
            sizeof(buffer.bytes)
        );

        if (length > 0) {
            continue;
        }
        if (length < 0 && errno == EINTR) {
            continue;
        }
        if (length < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }
        if (length == 0) {
            errno = EIO;
        }
        return false;
    }

    if (!refresh_directory_watch(watch) ||
        !config_file_state(watch->config_path, &current_state)) {
        return false;
    }

    *changed = !config_file_state_equal(&watch->file_state, &current_state);
    watch->file_state = current_state;
    return true;
}

void config_watch_destroy(ConfigWatch *watch) {
    if (!watch) {
        return;
    }
    if (watch->inotify_fd >= 0) {
        close(watch->inotify_fd);
    }
    free(watch);
}
