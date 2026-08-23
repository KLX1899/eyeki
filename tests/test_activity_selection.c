#define _POSIX_C_SOURCE 200809L

#include "activity_selection.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TARGET_UID ((uid_t)1000)
#define OTHER_UID ((uid_t)1001)

typedef struct {
    const char *id;
    uid_t uid;
    int active;
    int remote;
    const char *type;
    const char *session_class;
    bool property_error;
} FakeSession;

typedef struct {
    const char *process_session;
    int process_result;
    const char *display_session;
    int display_result;
    int sessions_result;
    const char *session_ids[8];
    FakeSession sessions[8];
    size_t session_count;
    int enumerate_calls;
} FakeLogind;

static FakeLogind fake;

static const FakeSession *find_session(const char *id) {
    for (size_t index = 0; index < fake.session_count; index++) {
        if (strcmp(fake.sessions[index].id, id) == 0) {
            return &fake.sessions[index];
        }
    }
    return NULL;
}

static int fake_pid_get_session(pid_t pid, char **session) {
    (void)pid;
    *session = NULL;
    if (fake.process_result >= 0) {
        *session = strdup(fake.process_session);
        return *session ? 0 : -ENOMEM;
    }
    return fake.process_result;
}

static int fake_uid_get_display(uid_t uid, char **session) {
    assert(uid == TARGET_UID);
    *session = NULL;
    if (fake.display_result >= 0) {
        *session = strdup(fake.display_session);
        return *session ? 0 : -ENOMEM;
    }
    return fake.display_result;
}

static int fake_uid_get_sessions(
    uid_t uid,
    int require_active,
    char ***sessions
) {
    char **copy;

    assert(uid == TARGET_UID);
    assert(require_active == 1);
    fake.enumerate_calls++;
    *sessions = NULL;
    if (fake.sessions_result < 0) {
        return fake.sessions_result;
    }
    if (fake.sessions_result == 0) {
        return 0;
    }

    copy = calloc((size_t)fake.sessions_result + 1, sizeof(*copy));
    assert(copy);
    for (int index = 0; index < fake.sessions_result; index++) {
        copy[index] = strdup(fake.session_ids[index]);
        assert(copy[index]);
    }
    *sessions = copy;
    return fake.sessions_result;
}

static int fake_session_get_uid(const char *session, uid_t *uid) {
    const FakeSession *value = find_session(session);

    if (!value || value->property_error) {
        return -EIO;
    }
    *uid = value->uid;
    return 0;
}

static int fake_session_is_active(const char *session) {
    const FakeSession *value = find_session(session);

    return !value || value->property_error ? -EIO : value->active;
}

static int fake_session_is_remote(const char *session) {
    const FakeSession *value = find_session(session);

    return !value || value->property_error ? -EIO : value->remote;
}

static int fake_session_get_type(const char *session, char **type) {
    const FakeSession *value = find_session(session);

    *type = NULL;
    if (!value || value->property_error) {
        return -EIO;
    }
    *type = strdup(value->type);
    return *type ? 0 : -ENOMEM;
}

static int fake_session_get_class(
    const char *session,
    char **session_class
) {
    const FakeSession *value = find_session(session);

    *session_class = NULL;
    if (!value || value->property_error) {
        return -EIO;
    }
    *session_class = strdup(value->session_class);
    return *session_class ? 0 : -ENOMEM;
}

static const ActivitySessionApi fake_api = {
    .pid_get_session = fake_pid_get_session,
    .uid_get_display = fake_uid_get_display,
    .uid_get_sessions = fake_uid_get_sessions,
    .session_get_uid = fake_session_get_uid,
    .session_is_active = fake_session_is_active,
    .session_is_remote = fake_session_is_remote,
    .session_get_type = fake_session_get_type,
    .session_get_class = fake_session_get_class
};

static FakeSession session(
    const char *id,
    uid_t uid,
    int active,
    int remote,
    const char *type,
    const char *session_class
) {
    FakeSession value = {
        .id = id,
        .uid = uid,
        .active = active,
        .remote = remote,
        .type = type,
        .session_class = session_class,
        .property_error = false
    };
    return value;
}

static void reset_fake(void) {
    memset(&fake, 0, sizeof(fake));
    fake.process_result = -ENODATA;
    fake.display_result = -ENXIO;
}

static void add_session(FakeSession value) {
    size_t index = fake.session_count;

    assert(index < sizeof(fake.sessions) / sizeof(fake.sessions[0]));
    fake.sessions[index] = value;
    fake.session_ids[index] = value.id;
    fake.session_count++;
    fake.sessions_result = (int)fake.session_count;
}

static ActivitySessionResult resolve(char **selected_session) {
    return activity_session_resolve(
        &fake_api,
        TARGET_UID,
        1234,
        selected_session
    );
}

static void test_process_session_is_authoritative(void) {
    char *selected = NULL;

    reset_fake();
    fake.process_result = 0;
    fake.process_session = "current";
    add_session(session(
        "current", TARGET_UID, 1, 0, "wayland", "user"
    ));
    add_session(session(
        "other", TARGET_UID, 1, 0, "x11", "user"
    ));

    assert(resolve(&selected) == ACTIVITY_SESSION_FOUND);
    assert(strcmp(selected, "current") == 0);
    assert(fake.enumerate_calls == 0);
    free(selected);
}

static void test_process_session_must_belong_to_target_uid(void) {
    char *selected = NULL;

    reset_fake();
    fake.process_result = 0;
    fake.process_session = "wrong-user";
    add_session(session(
        "wrong-user", OTHER_UID, 1, 0, "wayland", "user"
    ));

    assert(resolve(&selected) == ACTIVITY_SESSION_NONE);
    assert(!selected);
    assert(fake.enumerate_calls == 0);
}

static void test_primary_display_wins_for_user_service(void) {
    char *selected = NULL;

    reset_fake();
    fake.display_result = 0;
    fake.display_session = "display";
    add_session(session(
        "display", TARGET_UID, 1, 0, "wayland", "user"
    ));
    add_session(session(
        "second-seat", TARGET_UID, 1, 0, "x11", "user"
    ));

    assert(resolve(&selected) == ACTIVITY_SESSION_FOUND);
    assert(strcmp(selected, "display") == 0);
    free(selected);
}

static void test_only_local_active_graphical_user_session_is_selected(void) {
    char *selected = NULL;

    reset_fake();
    add_session(session(
        "wrong-user", OTHER_UID, 1, 0, "wayland", "user"
    ));
    add_session(session(
        "inactive", TARGET_UID, 0, 0, "wayland", "user"
    ));
    add_session(session(
        "remote", TARGET_UID, 1, 1, "wayland", "user"
    ));
    add_session(session(
        "tty", TARGET_UID, 1, 0, "tty", "user"
    ));
    add_session(session(
        "greeter", TARGET_UID, 1, 0, "wayland", "greeter"
    ));
    add_session(session(
        "eligible", TARGET_UID, 1, 0, "wayland", "user"
    ));

    assert(resolve(&selected) == ACTIVITY_SESSION_FOUND);
    assert(strcmp(selected, "eligible") == 0);
    free(selected);
}

static void test_multiple_sessions_without_primary_are_ambiguous(void) {
    char *selected = NULL;

    reset_fake();
    add_session(session(
        "seat-a", TARGET_UID, 1, 0, "wayland", "user"
    ));
    add_session(session(
        "seat-b", TARGET_UID, 1, 0, "x11", "user-light"
    ));

    assert(resolve(&selected) == ACTIVITY_SESSION_AMBIGUOUS);
    assert(!selected);
}

static void test_empty_and_error_results_are_distinct(void) {
    char *selected = NULL;

    reset_fake();
    assert(resolve(&selected) == ACTIVITY_SESSION_NONE);
    assert(!selected);

    reset_fake();
    fake.sessions_result = -EIO;
    assert(resolve(&selected) == ACTIVITY_SESSION_ERROR);
    assert(!selected);

    reset_fake();
    add_session(session(
        "broken", TARGET_UID, 1, 0, "wayland", "user"
    ));
    fake.sessions[0].property_error = true;
    assert(resolve(&selected) == ACTIVITY_SESSION_ERROR);
    assert(!selected);
}

int main(void) {
    test_process_session_is_authoritative();
    test_process_session_must_belong_to_target_uid();
    test_primary_display_wins_for_user_service();
    test_only_local_active_graphical_user_session_is_selected();
    test_multiple_sessions_without_primary_are_ambiguous();
    test_empty_and_error_results_are_distinct();
    puts("activity selection tests passed");
    return 0;
}
