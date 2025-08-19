/* KallistiOS ##version##

   dc/flashrom.h
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2008 Lawrence Sebald

*/

/** \file   dc/flashrom.h
    \brief  Dreamcast flashrom read/write support.

    This file implements wrappers for the BIOS flashrom syscalls, and some
    utilities to make it easier to use the flashrom info. Note that because the
    flash writing can be such a dangerous thing potentially (I haven't deleted
    my flash to see what happens, but given the info stored here it sounds like
    a Bad Idea(tm)), extreme care should be taken if you choose to use these
    functions!

    \author Megan Potter
    \author Lawrence Sebald
*/

#ifndef __DC_FLASHROM_H
#define __DC_FLASHROM_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \defgroup flashrom  Flashrom
    \brief              Driver for the Dreamcast's Internal Flash Storage
    \ingroup            vfs
*/

/** \defgroup fr_parts  Partitions
    \brief              Partitions available within the flashrom
    \ingroup            flashrom
    @{
*/
#define FLASHROM_PT_SYSTEM      0   /**< \brief Factory settings (read-only, 8K) */
#define FLASHROM_PT_RESERVED    1   /**< \brief reserved (all 0s, 8K) */
#define FLASHROM_PT_BLOCK_1     2   /**< \brief Block allocated (16K) */
#define FLASHROM_PT_SETTINGS    3   /**< \brief Game settings (block allocated, 32K) */
#define FLASHROM_PT_BLOCK_2     4   /**< \brief Block allocated (64K) */
/** @} */


/** \defgroup fr_blocks Logical Blocks
    \brief              Logical blocks available in the flashrom
    \ingroup            flashrom
    @{
*/
#define FLASHROM_B1_SYSCFG          0x05    /**< \brief System config (BLOCK_1) */
#define FLASHROM_B1_PW_SETTINGS_1   0x80    /**< \brief PlanetWeb settings (BLOCK_1) */
#define FLASHROM_B1_PW_SETTINGS_2   0x81    /**< \brief PlanetWeb settings (BLOCK_1) */
#define FLASHROM_B1_PW_SETTINGS_3   0x82    /**< \brief PlanetWeb settings (BLOCK_1) */
#define FLASHROM_B1_PW_SETTINGS_4   0x83    /**< \brief PlanetWeb settings (BLOCK_1) */
#define FLASHROM_B1_PW_SETTINGS_5   0x84    /**< \brief PlanetWeb settings (BLOCK_1) */
#define FLASHROM_B1_PW_PPP1         0xC0    /**< \brief PlanetWeb PPP settings (BLOCK_1) */
#define FLASHROM_B1_PW_PPP2         0xC1    /**< \brief PlanetWeb PPP settings (BLOCK_1) */
#define FLASHROM_B1_PW_DNS          0xC2    /**< \brief PlanetWeb DNS settings (BLOCK_1) */
#define FLASHROM_B1_PW_EMAIL1       0xC3    /**< \brief PlanetWeb Email settings (BLOCK_1) */
#define FLASHROM_B1_PW_EMAIL2       0xC4    /**< \brief PlanetWeb Email settings (BLOCK_1) */
#define FLASHROM_B1_PW_EMAIL_PROXY  0xC5    /**< \brief PlanetWeb Email/Proxy settings (BLOCK_1) */
#define FLASHROM_B1_DK_PPP1         0xC6    /**< \brief DreamKey PPP settings (also seen in PW) */
#define FLASHROM_B1_DK_PPP2         0xC7    /**< \brief DreamKey PPP settings (also seen in PW) */
#define FLASHROM_B1_DK_DNS          0xC8    /**< \brief DreamKey PPP settings (also seen in PW) */
#define FLASHROM_B1_IP_SETTINGS     0xE0    /**< \brief IP settings for BBA (BLOCK_1) */
#define FLASHROM_B1_EMAIL           0xE2    /**< \brief Email address (BLOCK_1) */
#define FLASHROM_B1_SMTP            0xE4    /**< \brief SMTP server setting (BLOCK_1) */
#define FLASHROM_B1_POP3            0xE5    /**< \brief POP3 server setting (BLOCK_1) */
#define FLASHROM_B1_POP3LOGIN       0xE6    /**< \brief POP3 login setting (BLOCK_1) */
#define FLASHROM_B1_POP3PASSWD      0xE7    /**< \brief POP3 password setting + proxy (BLOCK_1) */
#define FLASHROM_B1_PPPLOGIN        0xE8    /**< \brief PPP username + proxy (BLOCK_1) */
#define FLASHROM_B1_PPPPASSWD       0xE9    /**< \brief PPP passwd (BLOCK_1) */
#define FLASHROM_B1_PPPMODEM        0xEB    /**< \brief PPP modem settings */
/** @} */

#define FLASHROM_OFFSET_CRC         62      /**< \brief Location of CRC in each block */

/** \defgroup fr_errs   Error Values
    \brief              Error values for the flashrom_get_block() function
    \ingroup            flashrom
    @{
*/
#define FLASHROM_ERR_NONE           0       /**< \brief Success */
#define FLASHROM_ERR_NOT_FOUND      -1      /**< \brief Block not found */
#define FLASHROM_ERR_NO_PARTITION   -2      /**< \brief Partition not found */
#define FLASHROM_ERR_READ_PART      -3      /**< \brief Error reading partition */
#define FLASHROM_ERR_BAD_MAGIC      -4      /**< \brief Invalid block magic */
#define FLASHROM_ERR_BOGUS_PART     -5      /**< \brief Bogus partition size */
#define FLASHROM_ERR_NOMEM          -6      /**< \brief Memory allocation failure */
#define FLASHROM_ERR_READ_BITMAP    -7      /**< \brief Error reading bitmap */
#define FLASHROM_ERR_EMPTY_PART     -8      /**< \brief Empty partition */
#define FLASHROM_ERR_READ_BLOCK     -9      /**< \brief Error reading block */
/** @} */

/** \brief   Retrieve information about the given partition.
    \ingroup flashrom

    This function implements the FLASHROM_INFO syscall; given a partition ID,
    return two ints specifying the beginning and the size of the partition
    (respectively) inside the flashrom.

    \param  part            The partition ID in question.
    \param  start_out       Buffer for storing the start address.
    \param  size_out        Buffer for storing the size of the partition.
    \retval 0               On success.
    \retval -1              On error.
*/
int flashrom_info(int part, int * start_out, int * size_out);

/** \brief   Read data from the flashrom.
    \ingroup flashrom

    This function implements the FLASHROM_READ syscall; given a flashrom offset,
    an output buffer, and a count, this reads data from the flashrom.

    \param  offset          The offset to read from.
    \param  buffer_out      Space to read into.
    \param  bytes           The number of bytes to read.
    \return                 The number of bytes read if successful, or -1
                            otherwise.
*/
int flashrom_read(int offset, void * buffer_out, int bytes);

/** \brief   Write data to the flashrom.
    \ingroup flashrom

    This function implements the FLASHROM_WRITE syscall; given a flashrom
    offset, an input buffer, and a count, this writes data to the flashrom.

    \note It is not possible to write ones to the flashrom over zeros. If you
    want to do this, you must save the old data in the flashrom, delete it out,
    and save the new data back.

    \param  offset          The offset to write at.
    \param  buffer          The data to write.
    \param  bytes           The number of bytes to write.
    \return                 The number of bytes written if successful, -1
                            otherwise.
*/
int flashrom_write(int offset, void * buffer, int bytes);

/** \brief   Delete data from the flashrom.
    \ingroup flashrom

    This function implements the FLASHROM_DELETE syscall; given a partition
    offset, that entire partition of the flashrom will be deleted and all data
    will be reset to 0xFF bytes.

    \note This does not rewrite the magic block to the start of the partition.
    It is your responsibility to do this after running this function.

    \param  offset          The offset of the start of the partition to erase.
    \retval 0               On success.
    \retval -1              On error.
*/
int flashrom_delete(int offset);


/* Medium-level functions */
/** \brief   Get a logical block from the specified partition.
    \ingroup flashrom

    This function retrieves the specified block ID from the given partition. The
    newest version of the data is returned.

    \param  partid          The partition ID to look in.
    \param  blockid         The logical block ID to look for.
    \param  buffer_out      Space to store the data. Must be at least 60 bytes.
    \return                 0 on success, <0 on error.
    \see    fr_errs
*/
int flashrom_get_block(int partid, int blockid, uint8 * buffer_out);


/* Higher level functions */

/** \defgroup fr_langs  Language Settings
    \brief              Language settings possible in the BIOS menu
    \ingroup            flashrom

    This set of constants will be returned as the language value in the
    flashrom_syscfg_t structure.

    @{
*/
#define FLASHROM_LANG_JAPANESE  0   /**< \brief Japanese language code */
#define FLASHROM_LANG_ENGLISH   1   /**< \brief English language code */
#define FLASHROM_LANG_GERMAN    2   /**< \brief German language code */
#define FLASHROM_LANG_FRENCH    3   /**< \brief French language code */
#define FLASHROM_LANG_SPANISH   4   /**< \brief Spanish language code */
#define FLASHROM_LANG_ITALIAN   5   /**< \brief Italian language code */
/** @} */

/** \brief   System configuration structure.
    \ingroup flashrom

    This structure is filled in with the settings set in the BIOS from the
    flashrom_get_syscfg() function.

    \headerfile dc/flashrom.h
*/
typedef struct flashrom_syscfg {
    int language;   /**< \brief Language setting.
                         \see fr_langs */
    int audio;      /**< \brief Stereo/mono setting. 0 == mono, 1 == stereo */
    int autostart;  /**< \brief Autostart discs? 0 == off, 1 == on */
} flashrom_syscfg_t;

/** \brief   Retrieve the current system configuration settings.
    \ingroup flashrom

    \param  out             Storage for the configuration.
    \return                 0 on success, <0 on error.
    \see    fr_errs
*/
int flashrom_get_syscfg(flashrom_syscfg_t * out);


/** \defgroup fr_region Region Settings
    \brief              Region settings possible in the system
    \ingroup            flashrom

    One of these values should be returned by flashrom_get_region().

    @{
*/
#define FLASHROM_REGION_UNKNOWN 0   /**< \brief Unknown region */
#define FLASHROM_REGION_JAPAN   1   /**< \brief Japanese region */
#define FLASHROM_REGION_US      2   /**< \brief US/Canada region */
#define FLASHROM_REGION_EUROPE  3   /**< \brief European region */
/** @} */

/** \brief   Retrieve the console's region code.
    \ingroup flashrom

    This function attempts to find the region of the Dreamcast. It may or may
    not work on 100% of Dreamcasts, apparently.

    \return                 A region code (>=0), or error (<0).
    \see    fr_region
    \see    fr_errs
*/
int flashrom_get_region(void);

/** \defgroup fr_method Connection Methods
    \brief              Connection method types stored within flashrom
    \ingroup            flashrom

    These values are representative of what type of ISP is configured in the
    flashrom.

    @{
*/
#define FLASHROM_ISP_DIALUP 0   /**< \brief Dialup ISP */
#define FLASHROM_ISP_DHCP   1   /**< \brief DHCP-based ethernet */
#define FLASHROM_ISP_PPPOE  2   /**< \brief PPPoE-based ethernet */
#define FLASHROM_ISP_STATIC 3   /**< \brief Static IP-based ethernet */
/** @} */

/** \defgroup fr_fields ISP Config Fields
    \brief              Valid field constants for the flashrom_ispcfg_t struct
    \ingroup            flashrom

    The valid_fields field of the flashrom_ispcfg_t will have some combination
    of these ORed together to represent what data is filled in and believed
    valid.

    @{
*/
#define FLASHROM_ISP_IP         (1 <<  0)   /**< \brief Static IP address */
#define FLASHROM_ISP_NETMASK    (1 <<  1)   /**< \brief Netmask */
#define FLASHROM_ISP_BROADCAST  (1 <<  2)   /**< \brief Broadcast address */
#define FLASHROM_ISP_GATEWAY    (1 <<  3)   /**< \brief Gateway address */
#define FLASHROM_ISP_DNS        (1 <<  4)   /**< \brief DNS servers */
#define FLASHROM_ISP_HOSTNAME   (1 <<  5)   /**< \brief Hostname */
#define FLASHROM_ISP_EMAIL      (1 <<  6)   /**< \brief Email address */
#define FLASHROM_ISP_SMTP       (1 <<  7)   /**< \brief SMTP server */
#define FLASHROM_ISP_POP3       (1 <<  8)   /**< \brief POP3 server */
#define FLASHROM_ISP_POP3_USER  (1 <<  9)   /**< \brief POP3 username */
#define FLASHROM_ISP_POP3_PASS  (1 << 10)   /**< \brief POP3 password */
#define FLASHROM_ISP_PROXY_HOST (1 << 11)   /**< \brief Proxy hostname */
#define FLASHROM_ISP_PROXY_PORT (1 << 12)   /**< \brief Proxy port */
#define FLASHROM_ISP_PPP_USER   (1 << 13)   /**< \brief PPP username */
#define FLASHROM_ISP_PPP_PASS   (1 << 14)   /**< \brief PPP password */
#define FLASHROM_ISP_OUT_PREFIX (1 << 15)   /**< \brief Outside dial prefix */
#define FLASHROM_ISP_CW_PREFIX  (1 << 16)   /**< \brief Call waiting prefix */
#define FLASHROM_ISP_REAL_NAME  (1 << 17)   /**< \brief Real name */
#define FLASHROM_ISP_MODEM_INIT (1 << 18)   /**< \brief Modem init string */
#define FLASHROM_ISP_AREA_CODE  (1 << 19)   /**< \brief Area code */
#define FLASHROM_ISP_LD_PREFIX  (1 << 20)   /**< \brief Long distance prefix */
#define FLASHROM_ISP_PHONE1     (1 << 21)   /**< \brief Phone number 1 */
#define FLASHROM_ISP_PHONE2     (1 << 22)   /**< \brief Phone number 2 */
/** @} */

/** \defgroup fr_flags  ISP Config Flags
    \brief              Flags for the flashrom_ispcfg_t struct
    \ingroup            flashrom

    The flags field of the flashrom_ispcfg_t will have some combination of these
    ORed together to represent what settings were set.

    @{
*/
#define FLASHROM_ISP_DIAL_AREACODE  (1 <<  0)   /**< \brief Dial area code before number */
#define FLASHROM_ISP_USE_PROXY      (1 <<  1)   /**< \brief Proxy enabled */
#define FLASHROM_ISP_PULSE_DIAL     (1 <<  2)   /**< \brief Pulse dialing (instead of tone) */
#define FLASHROM_ISP_BLIND_DIAL     (1 <<  3)   /**< \brief Blind dial (don't wait for tone) */
/** @} */

/** \brief   ISP configuration structure.
    \ingroup flashrom

    This structure will be filled in by flashrom_get_ispcfg() (DreamPassport) or
    flashrom_get_pw_ispcfg() (PlanetWeb). Thanks to Sam Steele for the
    information about DreamPassport's ISP settings.

    \headerfile dc/flashrom.h
*/
typedef struct flashrom_ispcfg {
    int     method;         /**< \brief DHCP, Static, dialup(?), PPPoE
                                 \see   fr_method */
    uint32  valid_fields;   /**< \brief Which fields are valid?
                                 \see   fr_fields */
    uint32  flags;          /**< \brief Various flags that can be set in options
                                 \see   fr_flags */

    uint8   ip[4];          /**< \brief Host IP address */
    uint8   nm[4];          /**< \brief Netmask */
    uint8   bc[4];          /**< \brief Broadcast address */
    uint8   gw[4];          /**< \brief Gateway address */
    uint8   dns[2][4];      /**< \brief DNS servers (2) */
    int     proxy_port;     /**< \brief Proxy server port */
    char    hostname[24];   /**< \brief DHCP/Host name */
    char    email[64];      /**< \brief Email address */
    char    smtp[31];       /**< \brief SMTP server */
    char    pop3[31];       /**< \brief POP3 server */
    char    pop3_login[20]; /**< \brief POP3 login */
    char    pop3_passwd[32];/**< \brief POP3 passwd */
    char    proxy_host[31]; /**< \brief Proxy server hostname */
    char    ppp_login[29];  /**< \brief PPP login */
    char    ppp_passwd[20]; /**< \brief PPP password */
    char    out_prefix[9];  /**< \brief Outside dial prefix */
    char    cw_prefix[9];   /**< \brief Call waiting prefix */
    char    real_name[31];  /**< \brief The "Real Name" field of PlanetWeb */
    char    modem_init[33]; /**< \brief The modem init string to use */
    char    area_code[4];   /**< \brief The area code the user is in */
    char    ld_prefix[21];  /**< \brief The long-distance dial prefix */
    char    p1_areacode[4]; /**< \brief Phone number 1's area code */
    char    phone1[26];     /**< \brief Phone number 1 */
    char    p2_areacode[4]; /**< \brief Phone number 2's area code */
    char    phone2[26];     /**< \brief Phone number 2 */
} flashrom_ispcfg_t;

/** \brief   Retrieve DreamPassport's ISP configuration.
    \ingroup flashrom

    This function retrieves the console's ISP settings as set by DreamPassport,
    if they exist. You should check the valid_fields bitfield for the part of
    the struct you want before relying on the data.

    \param  out             Storage for the structure.
    \retval 0               On success.
    \retval -1              On error (no settings found, or other errors).
*/
int flashrom_get_ispcfg(flashrom_ispcfg_t * out);

/** \brief   Retrieve PlanetWeb's ISP configuration.
    \ingroup flashrom

    This function retrieves the console's ISP settings as set by PlanetWeb (1.0
    and 2.1 have been verified to work), if they exist. You should check the
    valid_fields bitfield for the part of the struct you want before relying on
    the data.

    \param  out             Storage for the structure.
    \retval 0               On success.
    \retval -1              On error (i.e, no PlanetWeb settings found).
*/
int flashrom_get_pw_ispcfg(flashrom_ispcfg_t *out);

/* More to come later */

__END_DECLS

#endif  /* __DC_FLASHROM_H */
