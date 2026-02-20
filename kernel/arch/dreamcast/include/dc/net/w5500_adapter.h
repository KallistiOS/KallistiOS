/* KallistiOS ##version##

   dc/net/w5500_adapter.h
   Copyright (C) 2025 Ruslan Rostovtsev

*/

/** \file    dc/net/w5500_adapter.h
    \brief   WIZnet W5500 network adapter support.
    \ingroup w5500_adapter

    This file contains declarations related to support for the WIZnet W5500
    network adapter on both SCI-SPI and SCIF-SPI interfaces.

    \author Ruslan Rostovtsev
*/

#ifndef __DC_NET_W5500_ADAPTER_H
#define __DC_NET_W5500_ADAPTER_H

#include <kos/cdefs.h>
#include <stdbool.h>
#include <stdint.h>

__BEGIN_DECLS

/** \defgroup w5500_adapter  W5500 Adapter
    \brief                   Driver for the WIZnet W5500 Adapter
    \ingroup                 networking_drivers
*/

/** \brief   Initialize the W5500 adapter.
    
    \param   mac_addr        Optional MAC address (NULL to use default)
    \param   use_thread      Enable background RX thread (true) or manual polling (false)
    \return                  0 on success, <0 on failure
*/
int w5500_adapter_init(const uint8_t *mac_addr, bool use_thread);

/** \brief   Shutdown the W5500 adapter.
    \return                  0 on success
*/
int w5500_adapter_shutdown(void);

__END_DECLS

#endif  /* __DC_NET_W5500_ADAPTER_H */
