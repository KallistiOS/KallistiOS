/* KallistiOS ##version##

   arpa/inet.h
   Copyright (C) 2006, 2007, 2010 Lawrence Sebald

*/

/** \file    arpa/inet.h
    \brief   Definitions for internet operations.
    \ingroup networking_utils

    This file contains the standard definitions (as directed by the POSIX 2008
    standard) for several internet-related functions.

    \author Lawrence Sebald
*/

#ifndef __ARPA_INET_H
#define __ARPA_INET_H

#include <kos/cdefs.h>

__BEGIN_DECLS

/* Bring in <netinet/in.h> to get the in_port_t, in_addr_t, and struct in_addr
   types. Bring in <inttypes.h> for uint32_t and uint16_t. IEEE Std 1003.1-2008
   specifically says that <arpa/inet.h> can make all the symbols from these
   headers visible. */
#include <netinet/in.h>
#include <inttypes.h>

/** \defgroup networking_utils  Utilities
    \brief                      Miscellaneous networking utilities
    \ingroup                    networking

    @{
*/

/** \brief  Convert a 32-bit value from host byte order to network byte order.
    \param  value           The value to convert.
    \return                 value converted to network byte order.
*/
uint32_t htonl(uint32_t value);

/** \brief  Convert a 32-bit value from network byte order to host byte order.
    \param  value           The value to convert.
    \return                 value converted to host byte order.
*/
uint32_t ntohl(uint32_t value);

/** \brief  Convert a 16-bit value from host byte order to network byte order.
    \param  value           The value to convert.
    \return                 value converted to network byte order.
*/
uint16_t htons(uint16_t value);

/** \brief  Convert a 16-bit value from network byte order to host byte order.
    \param  value           The value to convert.
    \return                 value converted to host byte order.
*/
uint16_t ntohs(uint16_t value);

/** \brief  Convert a string representation of an IPv4 address to an in_addr_t.

    This function converts a "dotted-decimal" string representation of an IPv4
    address to an in_addr_t for use in a struct in_addr. This function supports
    all POSIX-required formats for the representation of the address.

    \param  cp              A string representation of an IPv4 address.
    \return                 The binary representation of the requested IPv4
                            address. (in_addr_t)(-1) is returned on error.
*/
in_addr_t inet_addr(const char *cp);

/** \brief  Convert a string representation of an IPv4 address to a struct
            in_addr.

    This function, much like inet_addr, converts a string representation of an
    IPv4 address to a binary representation. This function, however, is
    non-standard (but seems to appear a lot of places). This function is a
    little nicer to work with than inet_addr simply because of the fact that the
    error return from inet_addr happens to actually correspond to a real IPv4
    address (255.255.255.255). This version actually distinguishes between that
    address and invalid addresses.

    \param  cp              A string representation of an IPv4 address.
    \param  pin             The destination for the conversion.
    \retval 0               An invalid IPv4 address was given.
    \retval 1               Upon successful conversion.
*/
int inet_aton(const char *cp, struct in_addr *pin);

/** \brief  Convert a string representation of an IP address to its binary
            representation.

    This function, like inet_addr, converts a string representation of an IP
    address to its binary representation. This function, unlike inet_aton, is
    actually standard (in POSIX 2008), and operates very similarly. The only
    differences between this function and inet_aton are that this function does
    not support hexadecimal or octal representations and that this function has
    the ability to support IPv6. This is the function that you should actually
    use to convert addresses from strings to binary in new code, rather than
    inet_addr or inet_aton.

    \param  af              The address family that src is an address in. The
                            only supported values are AF_INET and AF_INET6.
    \param  src             A string representation of the address.
    \param  dst             Storage for the result. For AF_INET, this must be at
                            least 32-bits in size (the function treats it as a
                            struct in_addr). For AF_INET6, this must be at least
                            128-bits in size (the function treats it as a struct
                            in6_addr).
    \retval -1              af is unsupported.
    \retval 0               An invalid address was given.
    \retval 1               Upon successful conversion.

    \par    Error Conditions:
    \em     EAFNOSUPPORT - the specified address family is unsupported
*/
int inet_pton(int af, const char *src, void *dst);

/** \brief  Convert a binary representation of an IP address to a string.

    This function does the exact opposite of the inet_pton function, converting
    a binary form of an address to a string. This function, unlike inet_ntoa, is
    reentrant, and is the function that you should generally use if you need to
    convert a binary representation of an IP address to a string.

    \param  af              The address family that src is in. The only
                            supported values are AF_INET and AF_INET6.
    \param  src             A binary representation of an IP address.
    \param  dst             Storage for the resulting string. This string should
                            be at least 16-bytes long for IPv4, and 46 bytes for
                            IPv6.
    \param  size            The length of dst.
    \retval NULL            Upon failed conversion.
    \retval dst             Upon successful conversion.

    \par    Error Conditions:
    \em     EAFNOSUPPORT - the specified address family is unsupported \n
    \em     ENOSPC - the size given is insufficient
*/
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);

/** \brief  Convert a binary representation of an IPv4 address to a string.

    This function does the exact opposite of the inet_addr function, converting
    a binary form of an address to a string. This function, unlike inet_ntop
    is non-reentrant (not thread-safe), and will always only support IPv4
    addresses. It is suggested to use inet_ntop in any new code.

    \param  addr            The address to convert.
    \return                 A string representation of addr (in dotted-decimal
                            form).
*/
char *inet_ntoa(struct in_addr addr);

/** @} */

__END_DECLS

#endif /* __ARPA_INET_H */
