#include "presentation.h"

#include <assert.h>
#include <stdio.h>

typedef struct {
    int notification_calls;
    int popup_calls;
    bool notification_succeeds;
    bool popup_succeeds;
} FakeBackends;

static bool initialize_notification(void *context) {
    FakeBackends *fake = context;

    fake->notification_calls++;
    return fake->notification_succeeds;
}

static bool initialize_popup(void *context) {
    FakeBackends *fake = context;

    fake->popup_calls++;
    return fake->popup_succeeds;
}

static PresentationBackends create_backends(FakeBackends *fake) {
    PresentationBackends backends;

    assert(presentation_backends_init(
        &backends,
        initialize_notification,
        fake,
        initialize_popup,
        fake
    ));
    return backends;
}

static void test_notification_to_popup_initializes_each_backend_once(void) {
    FakeBackends fake = {
        .notification_succeeds = true,
        .popup_succeeds = true
    };
    PresentationBackends backends = create_backends(&fake);

    assert(presentation_backends_prepare(&backends, MODE_NOTIFICATION));
    assert(presentation_backend_is_ready(&backends, MODE_NOTIFICATION));
    assert(!presentation_backend_is_ready(&backends, MODE_POPUP));
    assert(fake.notification_calls == 1);

    assert(presentation_backends_prepare(&backends, MODE_POPUP));
    assert(presentation_backend_is_ready(&backends, MODE_POPUP));
    assert(fake.popup_calls == 1);

    assert(presentation_backends_prepare(&backends, MODE_NOTIFICATION));
    assert(presentation_backends_prepare(&backends, MODE_POPUP));
    assert(fake.notification_calls == 1);
    assert(fake.popup_calls == 1);
}

static void test_popup_to_notification_initializes_each_backend(void) {
    FakeBackends fake = {
        .notification_succeeds = true,
        .popup_succeeds = true
    };
    PresentationBackends backends = create_backends(&fake);

    assert(presentation_backends_prepare(&backends, MODE_POPUP));
    assert(fake.popup_calls == 1);
    assert(fake.notification_calls == 0);

    assert(presentation_backends_prepare(&backends, MODE_NOTIFICATION));
    assert(fake.popup_calls == 1);
    assert(fake.notification_calls == 1);
}

static void test_failed_initialization_is_not_marked_ready(void) {
    FakeBackends fake = {
        .notification_succeeds = false,
        .popup_succeeds = false
    };
    PresentationBackends backends = create_backends(&fake);

    assert(!presentation_backends_prepare(&backends, MODE_NOTIFICATION));
    assert(!presentation_backend_is_ready(&backends, MODE_NOTIFICATION));
    assert(fake.notification_calls == 1);

    fake.notification_succeeds = true;
    assert(presentation_backends_prepare(&backends, MODE_NOTIFICATION));
    assert(presentation_backend_is_ready(&backends, MODE_NOTIFICATION));
    assert(fake.notification_calls == 2);

    assert(!presentation_backends_prepare(&backends, MODE_POPUP));
    assert(!presentation_backend_is_ready(&backends, MODE_POPUP));
    assert(fake.popup_calls == 1);

    fake.popup_succeeds = true;
    assert(presentation_backends_prepare(&backends, MODE_POPUP));
    assert(presentation_backend_is_ready(&backends, MODE_POPUP));
    assert(fake.popup_calls == 2);
}

static void test_invalid_arguments_are_rejected(void) {
    FakeBackends fake = {
        .notification_succeeds = true,
        .popup_succeeds = true
    };
    PresentationBackends backends;

    assert(!presentation_backends_init(
        NULL,
        initialize_notification,
        &fake,
        initialize_popup,
        &fake
    ));
    assert(!presentation_backends_init(
        &backends,
        NULL,
        &fake,
        initialize_popup,
        &fake
    ));
    assert(!presentation_backends_init(
        &backends,
        initialize_notification,
        &fake,
        NULL,
        &fake
    ));

    backends = create_backends(&fake);
    assert(!presentation_backends_prepare(&backends, (ReminderMode)99));
    assert(!presentation_backend_is_ready(&backends, (ReminderMode)99));
}

int main(void) {
    test_notification_to_popup_initializes_each_backend_once();
    test_popup_to_notification_initializes_each_backend();
    test_failed_initialization_is_not_marked_ready();
    test_invalid_arguments_are_rejected();
    puts("presentation tests passed");
    return 0;
}
