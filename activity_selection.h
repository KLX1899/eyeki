#ifndef EYEKI_ACTIVITY_SELECTION_H
#define EYEKI_ACTIVITY_SELECTION_H

#include <sys/types.h>

typedef enum {
    ACTIVITY_SESSION_FOUND,
    ACTIVITY_SESSION_NONE,
    ACTIVITY_SESSION_AMBIGUOUS,
    ACTIVITY_SESSION_ERROR
} ActivitySessionResult;

typedef struct {
    int (*pid_get_session)(pid_t pid, char **session);
    int (*uid_get_display)(uid_t uid, char **session);
    int (*uid_get_sessions)(uid_t uid, int require_active, char ***sessions);
    int (*session_get_uid)(const char *session, uid_t *uid);
    int (*session_is_active)(const char *session);
    int (*session_is_remote)(const char *session);
    int (*session_get_type)(const char *session, char **type);
    int (*session_get_class)(const char *session, char **session_class);
} ActivitySessionApi;

ActivitySessionResult activity_session_resolve(
    const ActivitySessionApi *api,
    uid_t target_uid,
    pid_t process_id,
    char **selected_session
);

#endif /* EYEKI_ACTIVITY_SELECTION_H */
