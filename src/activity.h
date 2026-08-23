#ifndef EYEKI_ACTIVITY_H
#define EYEKI_ACTIVITY_H

typedef enum {
    ACTIVITY_LOOKUP_OK,
    ACTIVITY_LOOKUP_NO_SESSION,
    ACTIVITY_LOOKUP_AMBIGUOUS,
    ACTIVITY_LOOKUP_ERROR
} ActivityLookupResult;

ActivityLookupResult activity_get_idle_seconds(int *idle_seconds);

#endif /* EYEKI_ACTIVITY_H */
