/* KallistiOS ##version##

   kernel/net/net_core.h
   Copyright (C) 2026 Paul Cercueil

*/

#ifndef __LOCAL_NET_CORE_H
#define __LOCAL_NET_CORE_H

#include <kos/cdefs.h>

__BEGIN_DECLS

#include <kos/workqueue.h>

extern workqueue_t *net_wq;

__END_DECLS

#endif /* !__LOCAL_NET_CORE_H */
