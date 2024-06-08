#include <kos/version.h>

kos_version_t kos_version(void) {
    return KOS_VERSION;
}

const char* kos_version_string(void) {
    return KOS_VERSION_STRING;
}

bool kos_version_min(uint8_t major, uint16_t minor, uint8_t patch) {
    return KOS_VERSION_MIN(major, minor, patch);
}

bool kos_version_equal(uint8_t major, uint16_t minor, uint8_t patch) {
    return KOS_VERSION_EQUAL(major, minor, patch);
}

bool kos_version_max(uint8_t major, uint16_t minor, uint8_t patch) {
    return KOS_VERSION_MAX(major, minor, patch);
}