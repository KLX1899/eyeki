#include "../config.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

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

int main(void) {
    test_interval_boundaries();
    test_malformed_and_out_of_range_intervals();
    test_conversion_rejects_invalid_values();
    puts("config tests passed");
    return 0;
}
