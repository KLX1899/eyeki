#define _POSIX_C_SOURCE 200809L

#include "../config.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static char *make_temp_directory(void) {
    char *path = strdup("/tmp/eyeki-config-test-XXXXXX");

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

static void assert_path_mode(const char *path, mode_t expected_mode) {
    struct stat status;

    assert(stat(path, &status) == 0);
    assert((status.st_mode & 0777) == expected_mode);
}

static void assert_file_contents(const char *path, const char *expected) {
    char contents[128];
    FILE *file = fopen(path, "r");
    size_t length;

    assert(file);
    length = fread(contents, 1, sizeof(contents) - 1, file);
    assert(!ferror(file));
    contents[length] = '\0';
    assert(fclose(file) == 0);
    assert(strcmp(contents, expected) == 0);
}

static void assert_config(Config cfg, int interval_minutes, ReminderMode mode) {
    assert(cfg.interval_minutes == interval_minutes);
    assert(cfg.mode == mode);
}

static int count_directory_entries(const char *path) {
    DIR *directory = opendir(path);
    struct dirent *entry;
    int count = 0;

    assert(directory);
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }
    assert(closedir(directory) == 0);
    return count;
}

static void assert_valid_interval(
    const char *value,
    int expected_minutes,
    uint64_t expected_seconds
) {
    int minutes = -1;
    uint64_t seconds = 0;

    assert(parse_interval_minutes(value, &minutes));
    assert(minutes == expected_minutes);
    assert(interval_minutes_to_seconds(minutes, &seconds));
    assert(seconds == expected_seconds);
}

static void assert_invalid_interval(const char *value) {
    int minutes = 123;

    assert(!parse_interval_minutes(value, &minutes));
    assert(minutes == 123);
}

static void test_interval_boundaries(void) {
    assert_valid_interval("10", 10, 600);
    assert_valid_interval("300", 300, 18000);
}

static void test_malformed_and_out_of_range_intervals(void) {
    assert_invalid_interval(NULL);
    assert_invalid_interval("");
    assert_invalid_interval("9");
    assert_invalid_interval("301");
    assert_invalid_interval("0");
    assert_invalid_interval("-10");
    assert_invalid_interval("+10");
    assert_invalid_interval(" 10");
    assert_invalid_interval("10 ");
    assert_invalid_interval("10minutes");
    assert_invalid_interval("999999999999999999999999999999999999999");
    assert(!parse_interval_minutes("10", NULL));
}

static void test_conversion_rejects_invalid_values(void) {
    uint64_t seconds = 123;

    assert(!interval_minutes_to_seconds(9, &seconds));
    assert(seconds == 123);
    assert(!interval_minutes_to_seconds(301, &seconds));
    assert(seconds == 123);
    assert(!interval_minutes_to_seconds(INT_MAX, &seconds));
    assert(seconds == 123);
    assert(!interval_minutes_to_seconds(INT_MIN, &seconds));
    assert(seconds == 123);
    assert(!interval_minutes_to_seconds(10, NULL));
}

static void test_fresh_home_round_trip_is_private_and_atomic(void) {
    char *home = make_temp_directory();
    char config_home[PATH_MAX];
    char config_directory[PATH_MAX];
    char config_path[PATH_MAX];
    char old_config_path[PATH_MAX];
    Config cfg = {10, MODE_NOTIFICATION};
    Config loaded;
    mode_t previous_umask;
    struct stat old_status;
    struct stat new_status;

    build_path(config_home, sizeof(config_home), home, ".config");
    build_path(
        config_directory,
        sizeof(config_directory),
        config_home,
        "eye_reminder"
    );
    build_path(config_path, sizeof(config_path), config_directory, "config");
    build_path(
        old_config_path,
        sizeof(old_config_path),
        config_directory,
        "old-config"
    );

    assert(setenv("HOME", home, 1) == 0);
    assert(unsetenv("XDG_CONFIG_HOME") == 0);
    previous_umask = umask(0777);
    assert(save_config(&cfg));
    umask(previous_umask);

    assert_path_mode(config_home, 0700);
    assert_path_mode(config_directory, 0700);
    assert_path_mode(config_path, 0600);
    assert_file_contents(
        config_path,
        "interval=10\nmode=notification\n"
    );
    loaded = load_config();
    assert_config(loaded, 10, MODE_NOTIFICATION);

    assert(link(config_path, old_config_path) == 0);
    assert(chmod(config_directory, 0755) == 0);
    assert(chmod(config_path, 0644) == 0);
    cfg.interval_minutes = 300;
    cfg.mode = MODE_POPUP;
    assert(save_config(&cfg));

    assert_path_mode(config_directory, 0700);
    assert_path_mode(config_path, 0600);
    assert_file_contents(config_path, "interval=300\nmode=popup\n");
    assert_file_contents(
        old_config_path,
        "interval=10\nmode=notification\n"
    );
    assert(stat(old_config_path, &old_status) == 0);
    assert(stat(config_path, &new_status) == 0);
    assert(old_status.st_ino != new_status.st_ino);
    assert(count_directory_entries(config_directory) == 2);

    assert(unlink(old_config_path) == 0);
    assert(unlink(config_path) == 0);
    assert(rmdir(config_directory) == 0);
    assert(rmdir(config_home) == 0);
    assert(rmdir(home) == 0);
    free(home);
}

static void test_xdg_precedence_and_legacy_fallback(void) {
    char *root = make_temp_directory();
    char home[PATH_MAX];
    char legacy_config_home[PATH_MAX];
    char legacy_directory[PATH_MAX];
    char legacy_path[PATH_MAX];
    char xdg_home[PATH_MAX];
    char xdg_parent[PATH_MAX];
    char xdg_directory[PATH_MAX];
    char xdg_path[PATH_MAX];
    Config legacy = {60, MODE_NOTIFICATION};
    Config migrated;

    build_path(home, sizeof(home), root, "home");
    build_path(legacy_config_home, sizeof(legacy_config_home), home, ".config");
    build_path(
        legacy_directory,
        sizeof(legacy_directory),
        legacy_config_home,
        "eye_reminder"
    );
    build_path(legacy_path, sizeof(legacy_path), legacy_directory, "config");
    build_path(xdg_parent, sizeof(xdg_parent), root, "xdg");
    build_path(xdg_home, sizeof(xdg_home), xdg_parent, "nested");
    build_path(
        xdg_directory,
        sizeof(xdg_directory),
        xdg_home,
        "eye_reminder"
    );
    build_path(xdg_path, sizeof(xdg_path), xdg_directory, "config");

    assert(mkdir(home, 0700) == 0);
    assert(setenv("HOME", home, 1) == 0);
    assert(unsetenv("XDG_CONFIG_HOME") == 0);
    assert(save_config(&legacy));

    assert(setenv("XDG_CONFIG_HOME", xdg_home, 1) == 0);
    assert_config(load_config(), 60, MODE_NOTIFICATION);
    assert(access(xdg_path, F_OK) < 0 && errno == ENOENT);

    migrated = load_config();
    migrated.interval_minutes = 300;
    migrated.mode = MODE_POPUP;
    assert(save_config(&migrated));
    assert_config(load_config(), 300, MODE_POPUP);

    assert(unsetenv("HOME") == 0);
    assert_config(load_config(), 300, MODE_POPUP);
    assert(save_config(&migrated));

    assert(setenv("HOME", home, 1) == 0);
    assert(unsetenv("XDG_CONFIG_HOME") == 0);
    assert_config(load_config(), 60, MODE_NOTIFICATION);
    assert(setenv("XDG_CONFIG_HOME", "relative/config", 1) == 0);
    assert_config(load_config(), 60, MODE_NOTIFICATION);
    assert(setenv("XDG_CONFIG_HOME", xdg_home, 1) == 0);
    assert_config(load_config(), 300, MODE_POPUP);

    assert(unlink(xdg_path) == 0);
    assert(rmdir(xdg_directory) == 0);
    assert(rmdir(xdg_home) == 0);
    assert(rmdir(xdg_parent) == 0);
    assert(unlink(legacy_path) == 0);
    assert(rmdir(legacy_directory) == 0);
    assert(rmdir(legacy_config_home) == 0);
    assert(rmdir(home) == 0);
    assert(rmdir(root) == 0);
    free(root);
}

static void test_save_failures_are_reported_and_cleaned_up(void) {
    Config cfg = {60, MODE_POPUP};
    char *root;
    char blocker[PATH_MAX];
    char app_directory[PATH_MAX];
    char target_directory[PATH_MAX];
    char config_path[PATH_MAX];
    FILE *file;

    assert(unsetenv("HOME") == 0);
    assert(unsetenv("XDG_CONFIG_HOME") == 0);
    errno = 0;
    assert(!save_config(&cfg));
    assert(errno == ENOENT);
    errno = 0;
    assert(!save_config(NULL));
    assert(errno == EINVAL);

    root = make_temp_directory();
    build_path(blocker, sizeof(blocker), root, "not-a-directory");
    file = fopen(blocker, "w");
    assert(file);
    assert(fclose(file) == 0);
    assert(setenv("XDG_CONFIG_HOME", blocker, 1) == 0);
    errno = 0;
    assert(!save_config(&cfg));
    assert(errno != 0);
    assert(unlink(blocker) == 0);
    assert(rmdir(root) == 0);
    free(root);

    root = make_temp_directory();
    build_path(app_directory, sizeof(app_directory), root, "eye_reminder");
    build_path(target_directory, sizeof(target_directory), root, "target");
    build_path(config_path, sizeof(config_path), target_directory, "config");
    assert(mkdir(target_directory, 0700) == 0);
    assert(symlink(target_directory, app_directory) == 0);
    assert(setenv("XDG_CONFIG_HOME", root, 1) == 0);
    errno = 0;
    assert(!save_config(&cfg));
    assert(errno != 0);
    assert(access(config_path, F_OK) < 0 && errno == ENOENT);
    assert(unlink(app_directory) == 0);
    assert(rmdir(target_directory) == 0);
    assert(rmdir(root) == 0);
    free(root);

    root = make_temp_directory();
    build_path(app_directory, sizeof(app_directory), root, "eye_reminder");
    build_path(config_path, sizeof(config_path), app_directory, "config");
    assert(mkdir(app_directory, 0700) == 0);
    assert(mkdir(config_path, 0700) == 0);
    assert(setenv("XDG_CONFIG_HOME", root, 1) == 0);
    errno = 0;
    assert(!save_config(&cfg));
    assert(errno != 0);
    assert(count_directory_entries(app_directory) == 1);
    assert(rmdir(config_path) == 0);
    assert(rmdir(app_directory) == 0);
    assert(rmdir(root) == 0);
    free(root);
}

int main(void) {
    test_interval_boundaries();
    test_malformed_and_out_of_range_intervals();
    test_conversion_rejects_invalid_values();
    test_fresh_home_round_trip_is_private_and_atomic();
    test_xdg_precedence_and_legacy_fallback();
    test_save_failures_are_reported_and_cleaned_up();
    puts("config tests passed");
    return 0;
}
