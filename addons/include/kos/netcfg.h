/* KallistiOS ##version##

   kos/netcfg.h
   Copyright (C) 2003 Megan Potter

*/

#ifndef __KOS_NETCFG_H
#define __KOS_NETCFG_H

/** \file    kos/netcfg.h
    \brief   Network configuration interface.
    \ingroup netcfg

    This file provides a common interface for reading and writing the network
    configuration on KOS. The interface can read from the flashrom on the
    Dreamcast or from a file (such as on a VMU or the like), and can write data
    back to a file.

    The data that is written out by this code is written in a relatively easy to
    parse text-based format.

    \author Megan Potter
*/

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

/** \defgroup   netcfg          Configuration
    \brief                      KOS Network Configuration Management
    \ingroup                    networking
*/

/** \defgroup   netcfg_methods  Methods
    \brief                      Network connection methods
    \ingroup                    netcfg

    These constants give the list of network connection methods that are
    supported by the netcfg_t type. One of these will be in the method field of
    objects of that type.

    @{
*/
#define NETCFG_METHOD_DHCP      0   /**< \brief Use DHCP to configure. */
#define NETCFG_METHOD_STATIC    1   /**< \brief Static network configuration. */
#define NETCFG_METHOD_PPPOE     4   /**< \brief Use PPPoE. */
/** @} */

/** \defgroup   netcfg_srcs     Sources
    \brief                      Network configuration sources
    \ingroup                    netcfg

    These constants give the list of places that the network configuration might
    be read from. One of these constants should be in the src field of objects
    of type netcfg_t.

    @{
*/
#define NETCFG_SRC_VMU      0   /**< \brief Read from a VMU. */
#define NETCFG_SRC_FLASH    1   /**< \brief Read from the flashrom. */
#define NETCFG_SRC_CWD      2   /**< \brief Read from the working directory. */
#define NETCFG_SRC_CDROOT   3   /**< \brief Read from the root of the CD. */
/** @} */

/** \brief   Network configuration information.
    \ingroup netcfg

    This structure contains information about the network configuration of the
    system, as set up by the user.

    \headerfile kos/netcfg.h
*/
typedef struct netcfg {
    /** \brief  Where was this configuration read from?
        \see    netcfg_srcs
    */
    int src;

    /** \brief  How should the network be configured?
        \see    netcfg_methods
    */
    int method;

    uint32_t ip;            /**< \brief IPv4 address of the console */
    uint32_t gateway;       /**< \brief IPv4 address of the gateway/router. */
    uint32_t netmask;       /**< \brief Network mask for the local net. */
    uint32_t broadcast;     /**< \brief Broadcast address for the local net. */
    uint32_t dns[2];        /**< \brief IPv4 address of the DNS servers. */
    char hostname[64];      /**< \brief DNS/DHCP hostname. */
    char email[64];         /**< \brief E-Mail address. */
    char smtp[64];          /**< \brief SMTP server address. */
    char pop3[64];          /**< \brief POP3 server address. */
    char pop3_login[64];    /**< \brief POP3 server username. */
    char pop3_passwd[64];   /**< \brief POP3 server password. */
    char proxy_host[64];    /**< \brief Proxy server address. */
    int proxy_port;         /**< \brief Proxy server port. */
    char ppp_login[64];     /**< \brief PPP Username. */
    char ppp_passwd[64];    /**< \brief PPP Password. */
    char driver[64];        /**< \brief Driver program filename (if any). */
} netcfg_t;

/** \brief   Load network configuration from a file.
    \ingroup netcfg

    This function loads the network configuration that is stored in the given
    file to the network configuration structure passed in. This function will
    also handle files on the VMU with the VMU specific header attached without
    any extra work required.

    \param  fn          The file to read the configuration from.
    \param  out         Buffer to store the parsed configuration.
    
    \return             0 on success, <0 on failure.
*/
int netcfg_load_from(const char *fn, netcfg_t *out);

/** \brief   Load network configuration from the Dreamcast's flashrom.
    \ingroup netcfg

    This function loads the network configuration that is stored in flashrom of
    the Dreamcast, parsing it into a netcfg_t.

    \note               This currently does not read the configuration stored by
                        the PlanetWeb browser at all.

    \param  out         Buffer to store the parsed configuration.
    
    \return             0 on success, <0 on failure.
*/
int netcfg_load_flash(netcfg_t *out);

/** \brief   Load network configuration.
    \ingroup netcfg

    This function loads the network configuration data, searching in multiple
    locations to attempt to find a file with a stored configuration. This
    function will attempt to read the configuration from each VMU first (from 
    a file named net.cfg), then it will try the flashrom, followed by the
    current working directory, and lastly the root of the CD.

    \param  out         Buffer to store the parsed configuration.
    
    \return             0 on success, <0 on failure.
*/
int netcfg_load(netcfg_t *out);

/** \brief   Save network configuration to a file.
    \ingroup netcfg

    This function saves the network configuration to the specified file. This
    function will automatically prepend the appropriate header if it is saved
    to a VMU.

    \param  fn          The file to save to.
    \param  cfg         The configuration to save.
    
    \return             0 on success, <0 on failure.
*/
int netcfg_save_to(const char *fn, const netcfg_t *cfg);

/** \brief   Save network configuration to the first available VMU.
    \ingroup netcfg

    This function saves the network configuration to first VMU that it finds. It
    will not retry if the first VMU doesn't have enough space to hold the file.

    \param  cfg         The configuration to save.
    
    \return             0 on success, <0 on failure.
*/
int netcfg_save(const netcfg_t *cfg);

__END_DECLS

#endif  /* __KOS_NETCFG_H */

