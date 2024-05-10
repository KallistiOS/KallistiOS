#ifndef __KOS_VERSION_H
#define __KOS_VERSION_H

#define KOS_VERSION_MAJOR   2
#define KOS_VERSION_MINOR   0
#define KOS_VERSION_PATCH   0

#define KOS_VERSION \
    KOS_VERSION_MAKE(KOS_VERSION_MAJOR, \
                     KOS_VERSION_MINOR, \
                     KOS_VERSION_PATCH)

#define KOS_VERSION_STRING \
    KOS_VERSION_MAKE_STRING(KOS_VERSION_MAJOR, \
                            KOS_VERSION_MINOR, \
                            KOS_VERSION_PATCH)

#define KOS_VERSION_MAKE(major, minor, patch) \
    (((kos_version_t)(major) << 24) | \
     ((kos_version_t)(minor) << 8)  | \
     ((kos_version_t)(patch)))

#define KOS_VERSION_MAKE_STRING(major, minor, patch) \
    KOS_STRINGIFY(major) "." \
    KOS_STRINGIFY(minor) "." \
    KOS_STRINGIFY(patch)

#define KOS_STRINGIFY(str) #str

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

typedef uint32_t kos_version_t;

kos_version_t kos_version(void);
const char* kos_version_string(void);

__END_DECLS

#endif /* __KOS_VERSION_H */