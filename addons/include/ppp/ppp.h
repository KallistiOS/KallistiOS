/* KallistiOS ##version##

   ppp/ppp.h
   Copyright (C) 2014 Lawrence Sebald
*/

/** \file    ppp/ppp.h
    \brief   PPP interface for network communications.
    \ingroup networking_ppp

    This file defines the API provided by libppp to interact with the PPP stack.
    PPP is a network communication protocol used to establish a direct link
    between two peers. It is most commonly used as the data link layer protocol
    for dialup internet access, but can also be potentially used on broadband
    connections (PPP over Ethernet or PPPoE) or on a direct serial line to
    a computer.

    The API presented by this library is designed to be extensible to whatever
    devices you might want to use it with, and was designed to integrate fairly
    simply into the rest of KOS' network stack.

    \author Lawrence Sebald
*/

#ifndef __PPP_PPP_H
#define __PPP_PPP_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <sys/types.h>
#include <sys/queue.h>

/** \defgroup networking_ppp    PPP
    \brief                      API for interacting with the Point-to-Point
                                Protocol stack
    \ingroup                    networking
*/

/** \brief   PPP device structure.
    \ingroup networking_ppp

    This structure defines a basic output device for PPP packets. This structure
    is largely modeled after netif_t from the main network stack, with a bit of
    functionality removed that is irrelevant for PPP.

    Note that we only allow one device and one connection in this library.

    \headerfile ppp/ppp.h
*/
typedef struct ppp_device {
    /** \brief  Device name ("modem", "scif", etc). */
    const char *name;

    /** \brief  Long description of the device. */
    const char *descr;

    /** \brief  Unit index (starts at zero and counts upwards for multiple
                network devices of the same type). */
    int index;

    /** \brief  Device flags.
        The lowest 16 bits of this value are reserved for use by libppp. You are
        free to use the other 16 bits as you see fit in your driver. */
    uint32_t flags;

    /** \brief  Private, device-specific data.
        This can be used for whatever the driver deems fit. The PPP code won't
        touch this data at all. Set to NULL if you don't need anything here. */
    void *privdata;

    /** \brief  Attempt to detect the device.
        \param  self        The network device in question.
        \return             0 on success, <0 on failure.
    */
    int (*detect)(struct ppp_device *self);

    /** \brief  Initialize the device.
        \param  self        The network device in question.
        \return             0 on success, <0 on failure.
    */
    int (*init)(struct ppp_device *self);

    /** \brief  Shutdown the device.
        \param  self        The network device in question.
        \return             0 on success, <0 on failure.
    */
    int (*shutdown)(struct ppp_device *self);

    /** \brief  Transmit data on the device.

        This function will be called periodically to transmit data on the
        underlying device. The data passed in may not necessarily be a whole
        packet (check the flags to see what's being passed in).

        \param  self        The network device in question.
        \param  data        The data to transmit.
        \param  len         The length of the data to transmit in bytes.
        \param  flags       Flags to describe what data is being sent in.
        \return             0 on success, <0 on failure.
    */
    int (*tx)(struct ppp_device *self, const uint8_t *data, size_t len,
              uint32_t flags);

    /** \brief  Poll for queued receive data.

        This function will be called periodically by a thread to check the
        device for any new incoming data.

        \param  self        The network device in question.
        \return             A pointer to the received data on success. NULL on
                            failure or if no data is waiting.
    */
    const uint8_t *(*rx)(struct ppp_device *self, ssize_t *out_len);
} ppp_device_t;

/** \brief   End of packet flag. 
    \ingroup networking_ppp
 */
#define PPP_TX_END_OF_PKT    0x00000001

/** \brief   PPP Protocol structure.
    \ingroup networking_ppp

    Each protocol that the PPP library can handle must have one of these
    registered. All protocols should be registered BEFORE attempting to actually
    establish a PPP session to ensure that each protocol can be used in the
    setup of the connection as needed.

    \headerfile ppp/ppp.h
*/
typedef struct ppp_proto {
    /** \brief  Protocol list entry (not a function!). */
    TAILQ_ENTRY(ppp_proto) entry;

    /** \brief  Protocol name ("lcp", "pap", etc). */
    const char *name;

    /** \brief  Protocol code. */
    uint16_t code;

    /** \brief  Private data (if any). */
    void *privdata;

    /** \brief  Initialization function.

        \param  self        The protocol structure for this protocol.
        \return             0 on success, <0 on failure.
        \note               Set to NULL if this is not needed in the protocol.
    */
    int (*init)(struct ppp_proto *self);

    /** \brief  Shutdown function.

        This function should perform any protocol-specific shutdown actions and
        unregister the protocol from the PPP protocol list.

        \param  self        The protocol structure for this protocol.
        \return             0 on success, <0 on failure.
    */
    int (*shutdown)(struct ppp_proto *self);

    /** \brief  Protocol packet input function.

        This function will be called for each packet delivered to the specified
        protocol.

        \param  self        The protocol structure for this protocol.
        \param  pkt         The packet being delivered.
        \param  len         The length of the packet in bytes.
        \return             0 on success, <0 on failure.
    */
    int (*input)(struct ppp_proto *self, const uint8_t *buf, size_t len);

    /** \brief  Notify the protocol of a PPP phase change.

        This function will be called by the PPP automaton any time that a phase
        change is initiated. This is often used for starting up a protocol when
        appropriate to do so (for instance, LCP uses this to begin negotiating
        configuration options with the peer when the establish phase is entered
        by the automaton).

        \param  self        The protocol structure for this protocol.
        \param  oldp        The old phase (the one the automaton is leaving).
        \param  newp        The new phase.
        \see                ppp_phases
    */
    void (*enter_phase)(struct ppp_proto *self, int oldp, int newp);

    /** \brief  Check timeouts for resending packets.

        This function will be called periodically to allow the protocol to check
        any resend timers that it might have responsibility for.

        \param  self        The protocol structure for this protocol.
        \param  tm          The current system time for checking timeouts
                            against (in milliseconds since system startup).
    */
    void (*check_timeouts)(struct ppp_proto *self, uint64_t tm);
} ppp_protocol_t;

/** \brief   Static initializer for protocol list entry. 
    \ingroup networking_ppp
 */
#define PPP_PROTO_ENTRY_INIT { NULL, NULL }

/** \defgroup ppp_phases                Automaton Phases
    \brief                              PPP automaton phases
    \ingroup                            networking_ppp

    This list defines the phases of the PPP automaton, as described in Section
    3.2 of RFC 1661.

    @{
*/
#define PPP_PHASE_DEAD          0x01    /**< \brief Pre-connection. */
#define PPP_PHASE_ESTABLISH     0x02    /**< \brief Establishing connection. */
#define PPP_PHASE_AUTHENTICATE  0x03    /**< \brief Authentication to peer. */
#define PPP_PHASE_NETWORK       0x04    /**< \brief Established and working. */
#define PPP_PHASE_TERMINATE     0x05    /**< \brief Tearing down the link. */
/** @} */

/** \brief   Set the device used to do PPP communications.
    \ingroup networking_ppp

    This function sets the device that further communications over a
    point-to-point link will take place over. The device need not be ready to
    communicate immediately upon calling this function.

    Unless you are adding support for a new device, you will probably never have
    to call this function. For instance, if you want to use the Dreamcast serial
    port to establish a link, the ppp_scif_init() function will call this for
    you.

    \warning            Calling this function after establishing a PPP link will
                        fail.

    \param  dev         The device to use for communication.
    
    \return             0 on success, <0 on failure.
*/
int ppp_set_device(ppp_device_t *dev);

/** \brief   Set the login credentials used to authenticate to the peer.
    \ingroup networking_ppp

    This function sets the login credentials that will be used to authenticate
    to the peer, if the peer requests authentication. The specifics of how the
    authentication takes place depends on what options are configured when
    establishing the link.

    \warning            Calling this function after establishing a PPP link will
                        fail.

    \param  username    The username to authenticate as.
    \param  password    The password to use to authenticate.
    
    \return             0 on success, <0 on failure.
*/
int ppp_set_login(const char *username, const char *password);

/** \brief   Send a packet on the PPP link.
    \ingroup networking_ppp

    This function sends a single packet to the peer on the PPP link. Generally,
    you should not use this function directly, but rather use the facilities
    provided by KOS' network stack.

    \param  data        The packet to send.
    \param  len         The length of the packet, in bytes.
    \param  proto       The PPP protocol number for the packet.
    
    \return             0 on success, <0 on failure.
*/
int ppp_send(const uint8_t *data, size_t len, uint16_t proto);

/** \brief   Register a protocol with the PPP stack.
    \ingroup networking_ppp

    This function adds a new protocol to the PPP stack, allowing the stack to
    communicate packets for the given protocol across the link. Generally, you
    should not have any reason to call this function, as the library includes
    a set of protocols to do normal communications.

    \param  hnd         A protocol handler structure.
 
    \return             0 on success, <0 on failure.
*/
int ppp_add_protocol(ppp_protocol_t *hnd);

/** \brief   Unregister a protocol from the PPP stack.
    \ingroup networking_ppp

    This function removes protocol from the PPP stack. This should be done at
    shutdown time of any protocols added with ppp_add_protocol().

    \param  hnd         A protocol handler structure.
    
    \return             0 on success, <0 on failure.
*/
int ppp_del_protocol(ppp_protocol_t *hnd);

/** \brief   Send a Protocol Reject packet on the link.
    \ingroup networking_ppp

    This function sends a LCP protocol reject packet on the link for the
    specified packet. Generally, you should not have to call this function, as
    the library will handle doing so internally.

    \param  proto       The PPP protocol number of the invalid packet.
    \param  pkt         The packet itself.
    \param  len         The length of the packet, in bytes.

    \return             0 on success, <0 on failure.
*/
int ppp_lcp_send_proto_reject(uint16_t proto, const uint8_t *pkt, size_t len);

/** \defgroup ppp_flags                 Configuration Flags
    \brief                              PPP link configuration flags
    \ingroup                            networking_ppp

    This list defines the flags we can negotiate during link establishment.

    @{
*/
#define PPP_FLAG_AUTH_PAP       0x00000001  /**< \brief PAP authentication */
#define PPP_FLAG_AUTH_CHAP      0x00000002  /**< \brief CHAP authentication */
#define PPP_FLAG_PCOMP          0x00000004  /**< \brief Protocol compression */
#define PPP_FLAG_ACCOMP         0x00000008  /**< \brief Addr/ctrl compression */
#define PPP_FLAG_MAGIC_NUMBER   0x00000010  /**< \brief Use magic numbers */
#define PPP_FLAG_WANT_MRU       0x00000020  /**< \brief Specify MRU */
#define PPP_FLAG_NO_ACCM        0x00000040  /**< \brief No ctl character map */
/** @} */

/** \brief   Get the flags set for our side of the link.
    \ingroup networking_ppp

    This function retrieves the connection flags set for our side of the PPP
    link. Before link establishment, this indicates the flags we would like to
    establish on the link and after establishment it represents the flags that
    were actually negotiated during link establishment.

    \return             Bitwise OR of \ref ppp_flags.
*/
uint32_t ppp_get_flags(void);

/** \brief   Get the flags set for the peer's side of the link.
    \ingroup networking_ppp

    This function retrieves the connection flags set for the other side of the
    PPP link. This value is only valid after link establishment.

    \return             Bitwise OR of \ref ppp_flags.
*/
uint32_t ppp_get_peer_flags(void);

/** \brief   Set the flags set for our side of the link.
    \ingroup networking_ppp

    \param flags        Bitwise combination of \ref ppp_flags.

    This function sets the connection flags for our side of the PPP link.
*/
void ppp_set_flags(uint32_t flags);

/** \brief   Establish a point-to-point link across a previously set-up device.
    \ingroup networking_ppp

    This function establishes a point-to-point link to the peer across a device
    that was previously set up with ppp_set_device(). Before calling this
    function, the device must be ready to communicate with the peer. That is to
    say, any handshaking needed to establish the underlying hardware link must
    have already completed.

    \return             0 on success, <0 on failure.

    \note               This function will block until the link is established.
*/
int ppp_connect(void);

/** \brief   Initialize the Dreamcast serial port for a PPP link.
    \ingroup networking_ppp

    This function sets up the Dreamcast serial port to act as a communications
    link for a point-to-point connection. This can be used in conjunction with a
    coder's cable (or similar) to connect the Dreamcast to a PC and the internet
    if the target system is set up properly.

    \param  bps         The speed to initialize the serial port at.

    \return             0 on success, <0 on failure.
*/
int ppp_scif_init(int bps);

/** \brief   Initialize the Dreamcast modem for a PPP link.
    \ingroup networking_ppp

    This function sets up the Dreamcast modem to act as a communications
    link for a point-to-point connection. This includes dialing the specified
    phone number and establishing the low-level link.

    \param  number      The phone number to dial out.
    \param  blind       Non-zero to blind dial (don't wait for a dial tone).
    \param  conn_rate   Storage for the connection rate, in bits per second. Set
                        to NULL if you do not need this value back from the
                        function.
    
    \retval 0           On success.
    \retval -1          If modem initialization fails.
    \retval -2          If not using blind dial and no dial tone is detected
                        within 5 seconds of opening the line.
    \retval -3          If dialing the modem fails.
    \retval -4          If a connection is not established in 60 seconds after
                        dialing.
*/
int ppp_modem_init(const char *number, int blind, int *conn_rate);

/** \brief   Initialize the PPP library.
    \ingroup networking_ppp

    This function initializes the PPP library, preparing internal structures for
    use and initializing the PPP protocols needed for normal IP communications.

    \return             0 on success, <0 on failure.
*/
int ppp_init(void);

/** \brief   Shut down the PPP library.
    \ingroup networking_ppp

    This function cleans up the PPP library, shutting down any connections and
    deinitializing any protocols that have bene registered previously.

    \return             0 on success, <0 on failure.
*/
int ppp_shutdown(void);

__END_DECLS
#endif /* !__PPP_PPP_H */
