#ifndef __KOS_INIT_BASE_H
#define __KOS_INIT_BASE_H

#define KOS_INIT_FLAG_WEAK(function, default_on) \
    static void (*function##_weak)(void) = NULL
#define KOS_INIT_FLAG_CALL(function) \
    (function##_weak ? (function##_weak(), 1) : 0)

#endif
