! This file is added to GCC during the patching stage of toolchain
! compilation. Any changes to this file will not take effect until the
! toolchain is recompiled.

! Weakly linked symbols used to get GCC to hopefully compile itself properly.
! These will be replaced by the real symbols in actual compiled programs.

    ! crt1.S required symbols
    .weak   ___kos_init_flags
    .weak   _arch_main
    .weak   __monstartup
    .weak   __executable_start
    .weak   __etext

    ! Things needed by emutls
    .weak   _free
    .weak   _abort
    .weak   _malloc
    .weak   _realloc
    .weak   _calloc

    ! gthr-kos.h required symbols
    .weak   _mutex_is_locked
    .weak   _mutex_destroy
    .weak   _mutex_lock
    .weak   _mutex_unlock
    .weak   _mutex_trylock
    .weak   _mutex_lock_timed
    .weak   _mutex_init

    .weak   _thd_create
    .weak   _thd_join
    .weak   _thd_detach
    .weak   _thd_pass
    .weak   _thd_exit
    .weak   _thd_get_current

    .weak   _kthread_setspecific
    .weak   _kthread_getspecific
    .weak   _kthread_key_create
    .weak   _kthread_key_delete
    .weak   _kthread_once

    .weak   _cond_destroy
    .weak   _cond_init
    .weak   _cond_wait
    .weak   _cond_wait_timed
    .weak   _cond_broadcast
    .weak   _cond_signal

___kos_init_flags:
    .long   0

_arch_main:
__monstartup:
_free:
_abort:
_malloc:
_realloc:
_calloc:
_mutex_is_locked:
_mutex_destroy:
_mutex_lock:
_mutex_unlock:
_mutex_trylock:
_mutex_lock_timed:
_mutex_init:
_thd_create:
_thd_join:
_thd_detach:
_thd_pass:
_thd_exit:
_thd_get_current:
_kthread_setspecific:
_kthread_getspecific:
_kthread_key_create:
_kthread_key_delete:
_kthread_once:
_cond_destroy:
_cond_init:
_cond_wait:
_cond_wait_timed:
_cond_broadcast:
_cond_signal:
    rts
    mov     #-1, r0
