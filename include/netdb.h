/* KallistiOS ##version##

   netdb.h
   Copyright (C) 2014 Lawrence Sebald

*/

/** \file    netdb.h
    \brief   Network address database functionality.
    \ingroup network_db

    This file contains functions related to network address lookups, usually
    performed through DNS.

    \author Lawrence Sebald
*/

#ifndef __NETDB_H
#define __NETDB_H

#include <kos/cdefs.h>

__BEGIN_DECLS

#include <netinet/in.h>
#include <sys/socket.h>
#include <inttypes.h>

/** \defgroup network_db    Address Database
    \brief                  Network Address Database Functionality
    \ingroup                networking
*/

/** \brief   Network host entry.
    \ingroup network_db

    This structure describes a network host entry in the address database. When
    looking up an address with the gethostbyname() function, one of these will
    be returned with information about the host.

    \headerfile netdb.h
*/
struct hostent {
    char *h_name;                   /**< \brief Official name of the host. */
    char **h_aliases;               /**< \brief Alternative host names. */
    int h_addrtype;                 /**< \brief Address type. */
    int h_length;                   /**< \brief Length of address, in bytes. */
    char **h_addr_list;             /**< \brief Network addresses of host. */
#define h_addr h_addr_list[0]       /**< \brief Primary network address. */
};

/** \brief   Network address information structure.
    \ingroup network_db

    This structure describes information on an address in the database. This
    structure is used by functions such as getaddrinfo() to return information
    about the specified host.

    \headerfile netdb.h
*/
struct addrinfo {
    int ai_flags;                   /**< \brief Input flags.
                                         \see   addrinfo_flags */
    int ai_family;                  /**< \brief Socket address family. */
    int ai_socktype;                /**< \brief Socket type. */
    int ai_protocol;                /**< \brief Socket protocol. */
    socklen_t ai_addrlen;           /**< \brief Address length. */
    struct sockaddr *ai_addr;       /**< \brief Address structure. */
    char *ai_canonname;             /**< \brief Canonical name. */
    struct addrinfo *ai_next;       /**< \brief Next address entry (if any). */
};

/** \brief   Error value for gethostbyname().
    \ingroup network_db
    \see     herrno_vals */
extern int h_errno;

/** \defgroup herrno_vals   h_errno Error Values
    \brief                  Error values for the h_errno variable
    \ingroup                network_db

    These are the possible values for h_errno, indicating errors returns from
    the gethostbyname() function.

    @{
*/
#define HOST_NOT_FOUND      1       /**< \brief Hostname not found. */
#define TRY_AGAIN           2       /**< \brief Try the request again. */
#define NO_RECOVERY         3       /**< \brief A non-recoverable error. */
#define NO_DATA             4       /**< \brief Host found, but no data. */
/** @} */

/** \defgroup addrinfo_errors   getaddrinfo() Error Values
    \brief                      Errors for the getaddrinfo() function
    \ingroup                    network_db

    These are the possible error return values from the getaddrinfo() function.

    @{
*/
#define EAI_AGAIN           1       /**< \brief Try the request again. */
#define EAI_BADFLAGS        2       /**< \brief Invalid hint flags. */
#define EAI_FAIL            3       /**< \brief A non-recoverable error. */
#define EAI_FAMILY          4       /**< \brief Invalid address family. */
#define EAI_MEMORY          5       /**< \brief Memory allocation error. */
#define EAI_NONAME          6       /**< \brief Hostname not found. */
#define EAI_SERVICE         7       /**< \brief Invalid service value. */
#define EAI_SOCKTYPE        8       /**< \brief Invalid socket type. */
#define EAI_SYSTEM          9       /**< \brief System error, check errno. */
#define EAI_OVERFLOW        10      /**< \brief Argument buffer overflow. */
/** @} */

/** \defgroup addrinfo_flags    addrinfo ai_flags
    \brief                      Flags for ai_flags in struct addrinfo
    \ingroup                    network_db

    These are the flags that can be set in the ai_flags field of struct
    addrinfo. These values can be bitwise ORed together.

    Currently only AI_PASSIVE is actually supported by the getaddrinfo()
    function.

    @{
*/
#define AI_PASSIVE      0x00000001  /**< \brief Address intended for bind(). */
#define AI_CANONNAME    0x00000002  /**< \brief Request canonical name. */
#define AI_NUMERICHOST  0x00000004  /**< \brief Inhibit host resolution. */
#define AI_NUMERICSERV  0x00000008  /**< \brief Inhibit service resolution. */
#define AI_V4MAPPED     0x00000010  /**< \brief Return v4-mapped IPv6 addrs. */
#define AI_ALL          0x00000020  /**< \brief Query for both IPv4 and IPv6. */
#define AI_ADDRCONFIG   0x00000040  /**< \brief Only query for IPv4/IPv6 addrs
                                                the system has a valid addr. */
/** @} */

/** \brief   Free an address information structure returned by getaddrinfo().
    \ingroup network_db

    This function cleans up any memory associated with the specified
    struct addrinfo, which was returned previously by a call to getaddrinfo().

    \param  ai              The struct addrinfo to clean up.
*/
void freeaddrinfo(struct addrinfo *ai);

/** \brief   Get information about a specified address.
    \ingroup network_db

    This function translates the name of a host and service into a set of socket
    addresses and related information to be used in creating a socket. This
    includes potentially looking up the host information in the network address
    database (and thus in DNS possibly as well).

    \param  nodename        The host to look up.
    \param  servname        The service to look up.
    \param  hints           Hints used in aiding lookup.
    \param  res             The resulting address information.
    \return                 0 on success, non-zero error code on failure.
    \see    addrinfo_errors
*/
int getaddrinfo(const char *nodename, const char *servname,
                const struct addrinfo *hints, struct addrinfo **res);

/** \brief   Look up a host by its name.
    \ingroup network_db

    This function queries the network address database (possibly recursively)
    for the network address of the specified hostname. This will first search
    any local databases before querying remote databases (such as a DNS server)
    for the host specified.

    \param  name            The hostname to look up.
    \return                 A pointer to a host entry on success or NULL on
                            failure. h_errno is set on failure to indicate the
                            error that occurred.

    \note                   This function is non-reentrant. getaddrinfo() should
                            (in general) be used instead of this function.
*/
struct hostent *gethostbyname(const char *name);

/** \brief   Look up a host by its name and address family.
    \ingroup network_db

    This function queries the network address database (possibly recursively)
    for the network address of the specified hostname. This will first search
    any local databases before querying remote databases (such as a DNS server)
    for the host specified.

    This function allows you to specify the address family that you wish the
    returned hostent to contain. This function is a GNU extension and has not
    been specified by any POSIX specification.

    \param  name            The hostname to look up.
    \param  af              The address family to use for lookups (AF_INET or
                            AF_INET6).
    \return                 A pointer to a host entry on success or NULL on
                            failure. h_errno is set on failure to indicate the
                            error that occurred.

    \note                   This function is non-reentrant. getaddrinfo() should
                            (in general) be used instead of this function.
*/
struct hostent *gethostbyname2(const char *name, int af);

__END_DECLS

#endif /* !__NETDB_H */
