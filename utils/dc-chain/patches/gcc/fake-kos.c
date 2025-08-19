/*
  This file is added to GCC during the patching stage of toolchain
  compilation. Any changes to this file will not take effect until the
  toolchain is recompiled.

  Weakly linked symbols used to get GCC to hopefully compile itself properly.
  These will be replaced by the real symbols in actual compiled programs.
*/

#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"

__attribute__((weak)) unsigned long __kos_init_flags;

__attribute__((unused)) int __fake_kos_fn(void) {
    return -1;
}

#define alias(fn) int fn(void) __attribute__((weak,alias("__fake_kos_fn")))

alias(arch_main);
alias(free);
alias(abort);
alias(malloc);
alias(realloc);
alias(calloc);
alias(mutex_is_locked);
alias(mutex_destroy);
alias(mutex_lock);
alias(mutex_unlock);
alias(mutex_trylock);
alias(mutex_lock_timed);
alias(mutex_init);
alias(thd_create);
alias(thd_join);
alias(thd_detach);
alias(thd_pass);
alias(thd_exit);
alias(thd_get_current);
alias(kthread_setspecific);
alias(kthread_getspecific);
alias(kthread_key_create);
alias(kthread_key_delete);
alias(kthread_once);
alias(cond_destroy);
alias(cond_init);
alias(cond_wait);
alias(cond_wait_timed);
alias(cond_broadcast);
alias(cond_signal);
alias(__newlib_lock_acquire_recursive);
alias(__newlib_lock_release_recursive);
alias(__newlib_lock_init_recursive);
alias(__newlib_lock_close_recursive);
alias(_malloc_r);
alias(_realloc_r);
alias(_free_r);
alias(_close_r);
alias(_write_r);
alias(_read_r);
alias(_lseek_r);
alias(_fstat_r);
alias(_isatty_r);
alias(_exit);
alias(__setup_argv_and_call_main);
