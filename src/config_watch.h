#ifndef EYEKI_CONFIG_WATCH_H
#define EYEKI_CONFIG_WATCH_H

#include <stdbool.h>

typedef struct ConfigWatch ConfigWatch;

ConfigWatch *config_watch_create(void);
int config_watch_fd(const ConfigWatch *watch);
bool config_watch_process(ConfigWatch *watch, bool *changed);
void config_watch_destroy(ConfigWatch *watch);

#endif /* EYEKI_CONFIG_WATCH_H */
