#include <kos/version.h>

kos_version_t kos_version(void) {
    return KOS_VERSION;
}

const char* kos_version_string(void) {
    return KOS_VERSION_STRING;
}