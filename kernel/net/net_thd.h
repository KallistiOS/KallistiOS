/* KallistiOS ##version##

   kernel/net/net_thd.h
   Copyright (C) 2009, 2012, 2013 Lawrence Sebald

*/

#ifndef __LOCAL_NET_THD_H
#define __LOCAL_NET_THD_H

#include <kos/cdefs.h>
#include <stdint.h>

__BEGIN_DECLS

int net_thd_add_callback(void (*cb)(void *), void *data, uint64_t timeout);
int net_thd_del_callback(int cbid);

int net_thd_is_current(void);

void net_thd_kill(void);

int net_thd_init(void);
void net_thd_shutdown(void);

__END_DECLS

#endif /* !__LOCAL_NET_THD_H */
