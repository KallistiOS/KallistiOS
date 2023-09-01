
#include <sys/lock.h>

void __libc_lock_init(_LOCK_T *lock) {
    return;
}

void __libc_lock_init_recursive(_LOCK_RECURSIVE_T *lock) {
    return;
}

void __libc_lock_close(_LOCK_T *lock) {
    return;
}

void __libc_lock_close_recursive(_LOCK_RECURSIVE_T *lock) {
    return;
}

void __libc_lock_acquire(_LOCK_T *lock) {
    return;
}

void __libc_lock_acquire_recursive(_LOCK_RECURSIVE_T *lock) {
    return;
}

void __libc_lock_release(_LOCK_T *lock) {
    return;
}

void __libc_lock_release_recursive(_LOCK_RECURSIVE_T *lock) {
    return;
}

int __libc_lock_try_acquire(_LOCK_T *lock) {
    return -1;
}

int __libc_lock_try_acquire_recursive(_LOCK_RECURSIVE_T *lock) {
    return -1;
}
