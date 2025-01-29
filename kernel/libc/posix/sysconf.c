/* KallistiOS ##version##

   sysconf.c
   Copyright (C) 2023, 2024, 2025 Falco Girgis
*/

#include <arch/arch.h>
#include <arch/cache.h>
#include <sys/uio.h>
#include <kos/netcfg.h>
#include <kos/fs.h>
#include <kos/thread.h>

#include <time.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

long sysconf(int name) {
    switch(name) {
        case _SC_HOST_NAME_MAX:  
            return sizeof ((netcfg_t *)NULL)->hostname;
        
        case _SC_CHILD_MAX:
            return 1;

        case _SC_CLK_TCK: 
            return thd_get_hz();
        
        case _SC_OPEN_MAX:
            return FD_SETSIZE;

        case _SC_ATEXIT_MAX:
            return UINT32_MAX;

        case _SC_IOV_MAX:
            return IOV_MAX;

        case _SC_MONOTONIC_CLOCK:
            return _POSIX_MONOTONIC_CLOCK;

        case _SC_CPUTIME:
            return _POSIX_CPUTIME;

        case _SC_THREAD_CPUTIME:
            return _POSIX_THREAD_CPUTIME;

        case _SC_PAGESIZE:
            return PAGESIZE;

        case _SC_LEVEL1_ICACHE_SIZE:
            return 8 * 1024;

        case _SC_LEVEL1_ICACHE_ASSOC:
            return 1;

        case _SC_LEVEL1_ICACHE_LINESIZE:
            return CPU_CACHE_BLOCK_SIZE;

        case _SC_LEVEL1_DCACHE_SIZE:
            return 16 * 1024;

        case _SC_LEVEL1_DCACHE_ASSOC:
            return 1;

        case _SC_LEVEL1_DCACHE_LINESIZE:
            return CPU_CACHE_BLOCK_SIZE;

        case _SC_SEM_NSEMS_MAX:
            return UINT32_MAX;

        case _SC_SEM_VALUE_MAX:
            return UINT32_MAX;
        
        case _SC_PHYS_PAGES:
            return page_count;
        
        case _SC_AVPHYS_PAGES:
            return mallinfo().fordblks / PAGESIZE;

        case _SC_NPROCESSORS_CONF: 
        case _SC_NPROCESSORS_ONLN: 
            return 1;

        default: 
            errno = EINVAL;
            return -1;
    }
}
