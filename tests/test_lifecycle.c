#include "lifecycle.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static void test_popup_close_reason(PopupCloseReason close_reason) {
    PopupLifecycle lifecycle;
    PopupCloseReason observed_reason = POPUP_CLOSE_NONE;

    popup_lifecycle_init(&lifecycle);
    assert(!popup_lifecycle_is_open(&lifecycle));
    assert(popup_lifecycle_open(&lifecycle));
    assert(popup_lifecycle_is_open(&lifecycle));
    assert(!popup_lifecycle_open(&lifecycle));

    assert(popup_lifecycle_dismiss(&lifecycle, close_reason));
    assert(!popup_lifecycle_is_open(&lifecycle));
    assert(!popup_lifecycle_dismiss(&lifecycle, close_reason));
    assert(!popup_lifecycle_open(&lifecycle));
    assert(popup_lifecycle_take_close(&lifecycle, &observed_reason));
    assert(observed_reason == close_reason);
    assert(!popup_lifecycle_take_close(&lifecycle, &observed_reason));
    assert(popup_lifecycle_open(&lifecycle));
}

static void test_all_popup_close_paths(void) {
    test_popup_close_reason(POPUP_CLOSE_ACKNOWLEDGED);
    test_popup_close_reason(POPUP_CLOSE_WINDOW_MANAGER);
    test_popup_close_reason(POPUP_CLOSE_MODE_CHANGE);
    test_popup_close_reason(POPUP_CLOSE_SHUTDOWN);
}

static void test_invalid_popup_lifecycle_operations(void) {
    PopupLifecycle lifecycle;
    PopupCloseReason reason = POPUP_CLOSE_NONE;

    popup_lifecycle_init(&lifecycle);
    assert(!popup_lifecycle_open(NULL));
    assert(!popup_lifecycle_dismiss(NULL, POPUP_CLOSE_ACKNOWLEDGED));
    assert(popup_lifecycle_open(&lifecycle));
    assert(!popup_lifecycle_dismiss(&lifecycle, POPUP_CLOSE_NONE));
    assert(!popup_lifecycle_dismiss(&lifecycle, (PopupCloseReason)99));
    assert(!popup_lifecycle_take_close(NULL, &reason));
    assert(!popup_lifecycle_take_close(&lifecycle, NULL));
    assert(!popup_lifecycle_is_open(NULL));
}

static void assert_signal_requests_shutdown(int signal_number) {
    int status;
    pid_t child = fork();

    assert(child >= 0);
    if (child == 0) {
        if (!lifecycle_install_signal_handlers() ||
            lifecycle_shutdown_requested() ||
            raise(signal_number) != 0 ||
            !lifecycle_shutdown_requested()) {
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
    }

    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == EXIT_SUCCESS);
}

static void test_termination_signals_request_shutdown(void) {
    assert_signal_requests_shutdown(SIGINT);
    assert_signal_requests_shutdown(SIGTERM);
}

int main(void) {
    test_all_popup_close_paths();
    test_invalid_popup_lifecycle_operations();
    test_termination_signals_request_shutdown();
    puts("lifecycle tests passed");
    return 0;
}
