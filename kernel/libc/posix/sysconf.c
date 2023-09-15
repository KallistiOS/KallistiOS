/* KallistiOS ##version##

   sysconf.c
   Copyright (C) 2023 Falco Girgis
*/

#include <arch/arch.h>
#include <kos/netcfg.h>
#include <kos/fs.h>

#include <malloc.h>
#include <unistd.h>
#include <errno.h>

long sysconf(int name) {
    switch(name) {
        case _SC_HOST_NAME_MAX:  
            return sizeof ((netcfg_t *)NULL)->hostname;
        
        case _SC_CLK_TCK: 
            return HZ;
        
        case _SC_OPEN_MAX:
            return FD_SETSIZE;

        case _SC_PAGESIZE:
            return PAGESIZE;
        
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
