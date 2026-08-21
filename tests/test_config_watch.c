#define _POSIX_C_SOURCE 200809L

#include "../config.h"
#include "../config_watch.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *make_temp_directory(void) {
    char *path = strdup("/tmp/eyeki-config-watch-test-XXXXXX");

    assert(path);
    assert(mkdtemp(path));
    return path;
}

static void build_path(
    char *path,
    size_t path_size,
    const char *base,
    const char *suffix
) {
    int result = snprintf(path, path_size, "%s/%s", base, suffix);

    assert(result >= 0);
    assert((size_t)result < path_size);
}

static void wait_for_change(ConfigWatch *watch) {
    struct pollfd descriptor = {
        .fd = config_watch_fd(watch),
        .events = POLLIN,
        .revents = 0
    };

    for (int attempt = 0; attempt < 10; attempt++) {
        bool changed;
        int result = poll(&descriptor, 1, 1000);

        assert(result == 1);
        assert(descriptor.revents & POLLIN);
        assert(!(descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)));
        assert(config_watch_process(watch, &changed));
        if (changed) {
            return;
        }
        descriptor.revents = 0;
    }

    assert(!"configuration change was not observed");
}

static void test_missing_config_then_atomic_updates_are_observed(void) {
    char *home = make_temp_directory();
    char config_home[PATH_MAX];
    char config_directory[PATH_MAX];
    char config_path[PATH_MAX];
    ConfigWatch *watch;
    Config first = {10, MODE_POPUP};
    Config second = {300, MODE_NOTIFICATION};
    Config loaded;

    assert(setenv("HOME", home, 1) == 0);
    assert(unsetenv("XDG_CONFIG_HOME") == 0);
    assert(config_get_path(config_path, sizeof(config_path)));
    build_path(config_home, sizeof(config_home), home, ".config");
    build_path(
        config_directory,
        sizeof(config_directory),
        config_home,
        "eye_reminder"
    );

    watch = config_watch_create();
    assert(watch);

    assert(save_config(&first));
    wait_for_change(watch);
    loaded = load_config();
    assert(loaded.interval_minutes == 10);
    assert(loaded.mode == MODE_POPUP);

    /* Replacing the file is a change even when values remain identical. */
    assert(save_config(&first));
    wait_for_change(watch);

    assert(save_config(&second));
    wait_for_change(watch);
    loaded = load_config();
    assert(loaded.interval_minutes == 300);
    assert(loaded.mode == MODE_NOTIFICATION);

    config_watch_destroy(watch);
    assert(unlink(config_path) == 0);
    assert(rmdir(config_directory) == 0);
    assert(rmdir(config_home) == 0);
    assert(rmdir(home) == 0);
    free(home);
}

static void test_xdg_creation_replaces_legacy_fallback(void) {
    char *root = make_temp_directory();
    char home[PATH_MAX];
    char legacy_home[PATH_MAX];
    char legacy_directory[PATH_MAX];
    char legacy_path[PATH_MAX];
    char xdg_home[PATH_MAX];
    char xdg_directory[PATH_MAX];
    char xdg_path[PATH_MAX];
    ConfigWatch *watch;
    Config legacy = {60, MODE_POPUP};
    Config migrated = {10, MODE_NOTIFICATION};
    Config loaded;

    build_path(home, sizeof(home), root, "home");
    build_path(legacy_home, sizeof(legacy_home), home, ".config");
    build_path(
        legacy_directory,
        sizeof(legacy_directory),
        legacy_home,
        "eye_reminder"
    );
    build_path(legacy_path, sizeof(legacy_path), legacy_directory, "config");
    build_path(xdg_home, sizeof(xdg_home), root, "xdg");
    build_path(
        xdg_directory,
        sizeof(xdg_directory),
        xdg_home,
        "eye_reminder"
    );
    build_path(xdg_path, sizeof(xdg_path), xdg_directory, "config");

    assert(mkdir(home, 0700) == 0);
    assert(mkdir(xdg_home, 0700) == 0);
    assert(setenv("HOME", home, 1) == 0);
    assert(unsetenv("XDG_CONFIG_HOME") == 0);
    assert(save_config(&legacy));

    assert(setenv("XDG_CONFIG_HOME", xdg_home, 1) == 0);
    watch = config_watch_create();
    assert(watch);
    loaded = load_config();
    assert(loaded.interval_minutes == 60);
    assert(loaded.mode == MODE_POPUP);

    assert(save_config(&migrated));
    wait_for_change(watch);
    loaded = load_config();
    assert(loaded.interval_minutes == 10);
    assert(loaded.mode == MODE_NOTIFICATION);

    config_watch_destroy(watch);
    assert(unlink(xdg_path) == 0);
    assert(rmdir(xdg_directory) == 0);
    assert(rmdir(xdg_home) == 0);
    assert(unlink(legacy_path) == 0);
    assert(rmdir(legacy_directory) == 0);
    assert(rmdir(legacy_home) == 0);
    assert(rmdir(home) == 0);
    assert(rmdir(root) == 0);
    free(root);
}

static void test_watch_requires_a_config_base(void) {
    ConfigWatch *watch;

    assert(unsetenv("HOME") == 0);
    assert(unsetenv("XDG_CONFIG_HOME") == 0);
    errno = 0;
    watch = config_watch_create();
    assert(!watch);
    assert(errno == ENOENT);
}

int main(void) {
    test_missing_config_then_atomic_updates_are_observed();
    test_xdg_creation_replaces_legacy_fallback();
    test_watch_requires_a_config_base();
    puts("config watch tests passed");
    return 0;
}
