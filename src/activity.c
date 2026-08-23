#include "activity.h"
#include "activity_selection.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-login.h>

static const ActivitySessionApi session_api = {
    .pid_get_session = sd_pid_get_session,
    .uid_get_display = sd_uid_get_display,
    .uid_get_sessions = sd_uid_get_sessions,
    .session_get_uid = sd_session_get_uid,
    .session_is_active = sd_session_is_active,
    .session_is_remote = sd_session_is_remote,
    .session_get_type = sd_session_get_type,
    .session_get_class = sd_session_get_class
};

static ActivityLookupResult map_session_result(
    ActivitySessionResult result
) {
    switch (result) {
        case ACTIVITY_SESSION_FOUND:
            return ACTIVITY_LOOKUP_OK;
        case ACTIVITY_SESSION_NONE:
            return ACTIVITY_LOOKUP_NO_SESSION;
        case ACTIVITY_SESSION_AMBIGUOUS:
            return ACTIVITY_LOOKUP_AMBIGUOUS;
        case ACTIVITY_SESSION_ERROR:
        default:
            return ACTIVITY_LOOKUP_ERROR;
    }
}

ActivityLookupResult activity_get_idle_seconds(int *idle_seconds) {
    sd_bus *bus = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    const char *session_path = NULL;
    char *session_id = NULL;
    ActivitySessionResult session_result;
    ActivityLookupResult result = ACTIVITY_LOOKUP_ERROR;
    int idle_hint = 0;
    uint64_t idle_since = 0;
    int status;

    if (!idle_seconds) {
        return ACTIVITY_LOOKUP_ERROR;
    }
    *idle_seconds = 0;

    session_result = activity_session_resolve(
        &session_api,
        geteuid(),
        0,
        &session_id
    );
    if (session_result != ACTIVITY_SESSION_FOUND) {
        return map_session_result(session_result);
    }

    status = sd_bus_open_system(&bus);
    if (status < 0) {
        goto cleanup;
    }

    status = sd_bus_call_method(
        bus,
        "org.freedesktop.login1",
        "/org/freedesktop/login1",
        "org.freedesktop.login1.Manager",
        "GetSession",
        &error,
        &reply,
        "s",
        session_id
    );
    if (status < 0 || !reply ||
        sd_bus_message_read(reply, "o", &session_path) < 0 ||
        !session_path) {
        goto cleanup;
    }

    status = sd_bus_get_property_trivial(
        bus,
        "org.freedesktop.login1",
        session_path,
        "org.freedesktop.login1.Session",
        "IdleHint",
        &error,
        'b',
        &idle_hint
    );
    if (status < 0) {
        goto cleanup;
    }
    if (!idle_hint) {
        result = ACTIVITY_LOOKUP_OK;
        goto cleanup;
    }

    sd_bus_error_free(&error);
    error = SD_BUS_ERROR_NULL;
    status = sd_bus_get_property_trivial(
        bus,
        "org.freedesktop.login1",
        session_path,
        "org.freedesktop.login1.Session",
        "IdleSinceHintMonotonic",
        &error,
        't',
        &idle_since
    );
    if (status < 0 || idle_since == 0) {
        goto cleanup;
    }

    {
        struct timespec now;
        uint64_t now_usec;
        uint64_t idle_elapsed;

        if (clock_gettime(CLOCK_MONOTONIC, &now) < 0 || now.tv_sec < 0 ||
            (uint64_t)now.tv_sec > UINT64_MAX / 1000000ULL) {
            goto cleanup;
        }
        now_usec = (uint64_t)now.tv_sec * 1000000ULL;
        now_usec += (uint64_t)now.tv_nsec / 1000ULL;
        if (idle_since > now_usec) {
            goto cleanup;
        }

        idle_elapsed = (now_usec - idle_since) / 1000000ULL;
        *idle_seconds = idle_elapsed > INT_MAX
            ? INT_MAX
            : (int)idle_elapsed;
    }
    result = ACTIVITY_LOOKUP_OK;

cleanup:
    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);
    sd_bus_unref(bus);
    free(session_id);
    return result;
}
