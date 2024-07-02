/* KallistiOS ##version##

   version.c
   Copyright (C) 2024 Falco Girgis
*/

/* This file contains the exported public symbols which can be used by user
    applications to query for the run-time version of KOS.
*/

#include <kos/version.h>

kos_version_t kos_version(void) {
    return KOS_VERSION;
}

const char *kos_version_string(void) {
    return KOS_VERSION_STRING;
}

bool kos_version_above(uint8_t major, uint16_t minor, uint8_t patch) {
    return KOS_VERSION_ABOVE(major, minor, patch);
}

bool kos_version_min(uint8_t major, uint16_t minor, uint8_t patch) {
    return KOS_VERSION_MIN(major, minor, patch);
}

bool kos_version_is(uint8_t major, uint16_t minor, uint8_t patch) {
    return KOS_VERSION_IS(major, minor, patch);
}

bool kos_version_max(uint8_t major, uint16_t minor, uint8_t patch) {
    return KOS_VERSION_MAX(major, minor, patch);
}

bool kos_version_below(uint8_t major, uint16_t minor, uint8_t patch) {
    return KOS_VERSION_BELOW(major, minor, patch);
}
