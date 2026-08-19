#ifndef EYEKI_CONFIG_H
#define EYEKI_CONFIG_H

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
void save_config(const Config *cfg);

#endif /* EYEKI_CONFIG_H */
