# KallistiOS ##version##
#
# addons/libpthread/Makefile
# Copyright (C) 2023 Lawrence Sebald
#

TARGET  = libpthread.a
SUBDIRS =

# Threads
OBJS  = pthread_create.o pthread_self.o pthread_detach.o pthread_join.o \
        pthread_exit.o pthread_equal.o pthread_getname_np.o \
        pthread_setname_np.o

# Thread Attributes
OBJS += pthread_attr_init.o pthread_attr_destroy.o \
        pthread_attr_setdetachstate.o pthread_attr_getdetachstate.o \
        pthread_attr_setstack.o pthread_attr_getstack.o \
        pthread_attr_setstacksize.o pthread_attr_getstacksize.o \
        pthread_attr_setschedparam.o pthread_attr_getschedparam.o \
        pthread_attr_setguardsize.o pthread_attr_getguardsize.o \
        pthread_attr_setscope.o pthread_attr_getscope.o \
        pthread_attr_setname_np.o pthread_attr_getname_np.o

# Thread Cancellation
OBJS += pthread_cancel.o pthread_testcancel.o pthread_setcancelstate.o \
        pthread_setcanceltype.o

# Dynamic package initialization
OBJS += pthread_once.o

# Thread-specific data key creation/deletion and data management
OBJS += pthread_key_create.o pthread_key_delete.o pthread_getspecific.o \
        pthread_setspecific.o

# Mutexes
OBJS += pthread_mutex_init.o pthread_mutex_destroy.o pthread_mutex_lock.o \
        pthread_mutex_trylock.o pthread_mutex_timedlock.o \
        pthread_mutex_consistent.o \
        pthread_mutexattr_init.o pthread_mutexattr_destroy.o \
        pthread_mutexattr_settype.o pthread_mutexattr_gettype.o \
        pthread_mutexattr_setrobust.o pthread_mutexattr_getrobust.o

# Condition Variables
OBJS += pthread_cond_init.o pthread_cond_destroy.o pthread_cond_wait.o \
        pthread_cond_timedwait.o pthread_cond_signal.o \
        pthread_cond_broadcast.o \
        pthread_condattr_init.o pthread_condattr_destroy.o \
        pthread_condattr_setclock.o pthread_condattr_getclock.o

# Reader/Writer Locks
OBJS += pthread_rwlockattr_init.o pthread_rwlockattr_destroy.o \
        pthread_rwlock_init.o pthread_rwlock_destroy.o pthread_rwlock_rdlock.o \
        pthread_rwlock_wrlock.o pthread_rwlock_unlock.o \
        pthread_rwlock_tryrdlock.o pthread_rwlock_trywrlock.o \
        pthread_rwlock_timedrdlock.o pthread_rwlock_timedwrlock.o

# Spin Locks
OBJS += pthread_spin_init.o pthread_spin_destroy.o pthread_spin_lock.o \
        pthread_spin_trylock.o pthread_spin_unlock.o

# Misc.
OBJS += pthread_atfork.o pthread_getsetconcurrency.o

include $(KOS_BASE)/addons/Makefile.prefab
