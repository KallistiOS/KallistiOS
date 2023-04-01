/* KallistiOS ##version##

   dc/net/lan_adapter.h
   Copyright (C) 2002 Megan Potter

*/

/** \file   dc/net/lan_adapter.h
    \brief  LAN Adapter support.

    This file contains declarations related to support for the HIT-0300 "LAN
    Adapter". There's not really anything that users will generally have to deal
    with in here.

    \author Megan Potter
*/

#ifndef __DC_NET_LAN_ADAPTER_H
#define __DC_NET_LAN_ADAPTER_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/net.h>

/* \cond */
/* Initialize */
int la_init();

/* Shutdown */
int la_shutdown();
/* \endcond */

__END_DECLS

#endif  /* __DC_NET_LAN_ADAPTER_H */

