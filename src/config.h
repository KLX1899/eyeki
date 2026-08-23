#ifndef EYEKI_CONFIG_H
#define EYEKI_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EYEKI_MIN_INTERVAL_MINUTES 10
#define EYEKI_MAX_INTERVAL_MINUTES 300

typedef enum {
    MODE_NOTIFICATION,
    MODE_POPUP
} ReminderMode;

typedef struct {
    int interval_minutes;
    ReminderMode mode;
} Config;

Config default_config(void);
Config load_config(void);
bool config_get_path(char *path, size_t path_size);
/* Returns false and leaves the filesystem failure in errno. */
bool save_config(const Config *cfg);
bool parse_interval_minutes(const char *value, int *interval_minutes);
bool interval_minutes_to_seconds(int interval_minutes, uint64_t *seconds);

#endif /* EYEKI_CONFIG_H */
