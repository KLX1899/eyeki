#define _POSIX_C_SOURCE 200809L

#include "activity_selection.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static bool is_expected_absence(int result) {
    return result == -ENODATA || result == -ENXIO;
}

static bool is_graphical_type(const char *type) {
    return strcmp(type, "x11") == 0 ||
        strcmp(type, "wayland") == 0 ||
        strcmp(type, "mir") == 0;
}

static bool is_user_class(const char *session_class) {
    return strcmp(session_class, "user") == 0 ||
        strncmp(session_class, "user-", 5) == 0;
}

static ActivitySessionResult inspect_session(
    const ActivitySessionApi *api,
    const char *session,
    uid_t target_uid
) {
    char *session_class = NULL;
    char *type = NULL;
    uid_t session_uid;
    int active;
    int remote;
    ActivitySessionResult result = ACTIVITY_SESSION_ERROR;

    if (api->session_get_uid(session, &session_uid) < 0) {
        goto cleanup;
    }
    if (session_uid != target_uid) {
        result = ACTIVITY_SESSION_NONE;
        goto cleanup;
    }

    active = api->session_is_active(session);
    if (active < 0) {
        goto cleanup;
    }
    if (active == 0) {
        result = ACTIVITY_SESSION_NONE;
        goto cleanup;
    }

    remote = api->session_is_remote(session);
    if (remote < 0) {
        goto cleanup;
    }
    if (remote > 0) {
        result = ACTIVITY_SESSION_NONE;
        goto cleanup;
    }

    if (api->session_get_type(session, &type) < 0 || !type) {
        goto cleanup;
    }
    if (!is_graphical_type(type)) {
        result = ACTIVITY_SESSION_NONE;
        goto cleanup;
    }

    if (api->session_get_class(session, &session_class) < 0 ||
        !session_class) {
        goto cleanup;
    }
    result = is_user_class(session_class)
        ? ACTIVITY_SESSION_FOUND
        : ACTIVITY_SESSION_NONE;

cleanup:
    free(session_class);
    free(type);
    return result;
}

static void free_string_array(char **values) {
    if (!values) {
        return;
    }

    for (size_t index = 0; values[index]; index++) {
        free(values[index]);
    }
    free(values);
}

static ActivitySessionResult copy_session(
    const char *session,
    char **selected_session
) {
    *selected_session = strdup(session);
    return *selected_session
        ? ACTIVITY_SESSION_FOUND
        : ACTIVITY_SESSION_ERROR;
}

ActivitySessionResult activity_session_resolve(
    const ActivitySessionApi *api,
    uid_t target_uid,
    pid_t process_id,
    char **selected_session
) {
    char *process_session = NULL;
    char *display_session = NULL;
    char **sessions = NULL;
    const char *first_eligible = NULL;
    const char *display_eligible = NULL;
    size_t eligible_count = 0;
    bool candidate_error = false;
    bool display_error = false;
    ActivitySessionResult result;
    int session_count;
    int status;

    if (!api || !selected_session || !api->pid_get_session ||
        !api->uid_get_display || !api->uid_get_sessions ||
        !api->session_get_uid || !api->session_is_active ||
        !api->session_is_remote || !api->session_get_type ||
        !api->session_get_class) {
        return ACTIVITY_SESSION_ERROR;
    }
    *selected_session = NULL;

    status = api->pid_get_session(process_id, &process_session);
    if (status >= 0) {
        if (!process_session) {
            return ACTIVITY_SESSION_ERROR;
        }
        result = inspect_session(api, process_session, target_uid);
        if (result == ACTIVITY_SESSION_FOUND) {
            result = copy_session(process_session, selected_session);
        }
        free(process_session);
        return result;
    }
    free(process_session);
    if (!is_expected_absence(status)) {
        return ACTIVITY_SESSION_ERROR;
    }

    status = api->uid_get_display(target_uid, &display_session);
    if (status < 0) {
        display_error = !is_expected_absence(status);
        free(display_session);
        display_session = NULL;
    } else if (!display_session) {
        display_error = true;
    }

    session_count = api->uid_get_sessions(target_uid, 1, &sessions);
    if (session_count < 0 || (session_count > 0 && !sessions)) {
        result = ACTIVITY_SESSION_ERROR;
        goto cleanup;
    }
    if (session_count == 0) {
        result = ACTIVITY_SESSION_NONE;
        goto cleanup;
    }

    for (int index = 0; index < session_count; index++) {
        ActivitySessionResult candidate_result;

        if (!sessions[index]) {
            candidate_error = true;
            break;
        }

        candidate_result = inspect_session(
            api,
            sessions[index],
            target_uid
        );
        if (candidate_result == ACTIVITY_SESSION_ERROR) {
            candidate_error = true;
            continue;
        }
        if (candidate_result != ACTIVITY_SESSION_FOUND) {
            continue;
        }

        if (!first_eligible) {
            first_eligible = sessions[index];
        }
        eligible_count++;
        if (display_session &&
            strcmp(display_session, sessions[index]) == 0) {
            display_eligible = sessions[index];
        }
    }

    if (display_eligible) {
        result = copy_session(display_eligible, selected_session);
    } else if (candidate_error) {
        result = ACTIVITY_SESSION_ERROR;
    } else if (eligible_count == 0) {
        result = ACTIVITY_SESSION_NONE;
    } else if (eligible_count == 1) {
        result = copy_session(first_eligible, selected_session);
    } else if (display_error) {
        result = ACTIVITY_SESSION_ERROR;
    } else {
        result = ACTIVITY_SESSION_AMBIGUOUS;
    }

cleanup:
    free_string_array(sessions);
    free(display_session);
    return result;
}
