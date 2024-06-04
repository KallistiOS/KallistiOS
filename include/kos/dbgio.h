/* KallistiOS ##version##

   kos/include/dbgio.h
   Copyright (C) 2000, 2004 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

/** \file    kos/dbgio.h
    \brief   Debug I/O.
    \ingroup logging_dbgio

    This file contains the Debug I/O system, which abstracts things so that
    various types of debugging tools can be used by programs in KOS. Included
    among these tools is the dcload console (dcload-serial, dcload-ip, and
    fs_dclsocket), a raw serial console, and a framebuffer-based console.

    \author Megan Potter
    \author Falco Girgis
*/

#ifndef __KOS_DBGIO_H
#define __KOS_DBGIO_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/queue.h>

/** \defgroup logging_dbgio     Debug I/O
    \brief    Debug I/O System and Abstraction  
    \ingroup  logging

    @{
*/

/** \brief IRQ mode to use with Debug I/O. 

    Two different IRQ modes which may be supported by a dbgio_handler.

    \sa    dbgio_set_irq_usage()
*/
typedef enum dbgio_mode {
    DBGIO_MODE_POLLED = 0, /**< Polled I/O mode */
    DBGIO_MODE_IRQ    = 1  /**< IRQ-based I/O mode */
} dbgio_mode_t;

/** \cond */
LIST_HEAD(dbgio_list, dbgio_handler);
/** \endcond */

/** \brief   Debug I/O Driver Interface.

    This struct represents a single dbgio interface. This should represent
    a generic pollable console interface. We will store an ordered list of
    these statically linked into the program and fall back from one to the
    next until one returns true for detected(). Note that the last device in
    this chain is the null console, which will always return true.
*/
typedef struct dbgio_handler {
    /** \brief  Name of the dbgio handler */
    const char  *name;

    /** \brief  Detect this debug interface.
        \retval true        If the device is available and usable
        \retval false       If the device is unavailable
    */
    bool (*detected)(void);

    /** \brief  Initialize this debug interface with default parameters.
        \retval 0           On success
        \retval -1          On failure
    */
    int (*init)(void);

    /** \brief  Shutdown this debug interface.
        \retval 0           On success
        \retval -1          On failure
    */
    int (*shutdown)(void);

    /** \brief  Set either polled or IRQ usage for this interface.
        \param  mode        Desired IRQ mode for the interface.
        \retval 0           On success
        \retval -1          On failure
    */
    int (*set_irq_usage)(dbgio_mode_t mode);

    /** \brief  Read one character from the console.
        \retval 0           On success
        \retval -1          On failure (set errno as appropriate)
    */
    int (*read)(void);

    /** \brief  Write one character to the console.
        \param  c           The character to write
        \retval 1           On success
        \retval -1          On error (set errno as appropriate)
        \note               Interfaces may require a call to flush() before the
                            output is actually flushed to the console.
    */
    int (*write)(int c);

    /** \brief  Flush any queued output.
        \retval 0           On success
        \retval -1          On error (set errno as appropriate)
    */
    int (*flush)(void);

    /** \brief  Write an entire buffer of data to the console.
        \param  data        The buffer to write
        \param  len         The length of the buffer
        \param  xlat        If non-zero, newline transformations may occur
        \return             Number of characters written on success, or -1 on
                            failure (set errno as appropriate)
    */
    int (*write_buffer)(const uint8_t *data, size_t len, bool xlat);

    /** \brief  Read an entire buffer of data from the console.
        \param  data        The buffer to read into
        \param  len         The length of the buffer
        \return             Number of characters read on success, or -1 on
                            failure (set errno as appropriate)
    */
    int (*read_buffer)(uint8_t *data, size_t len);

    /** \brief List of registered dbgio_handlers. 
    
        This member is managed internally and doesn't need to be set.
    */
    LIST_ENTRY(dbgio_handler) registry;
} dbgio_handler_t;

int dbgio_register(dbgio_handler_t *handler);

int dbgio_unregister(dbgio_handler_t *handler);

dbgio_handler_t *dbgio_find(const char *name);

int dbgio_aux_select(const char* name);
const char *dbgio_aux_get(void);

/** \brief   Select a new dbgio interface by name.

    This function manually selects a new dbgio interface by name. This function
    will allow you to select a device, even if it is not detected.

    \param  name            The dbgio interface to select
    \retval 0               On success
    
    \retval -1              On error

    \par    Error Conditions:
    \em     ENODEV - The specified device could not be initialized
*/
int dbgio_dev_select(const char *name);

/** \brief   Fetch the name of the currently selected dbgio interface.

    \return                 The name of the current dbgio interface (or NULL if
                            no device is selected)
*/
const char *dbgio_dev_get(void);

/** \brief   Initialize the dbgio console.

    This function is called internally, and shouldn't need to be called by any
    user programs.

    \retval 0               On success
    
    \retval -1              On error
    
    \par    Error Conditions:
    \em     ENODEV - No devices could be detected/initialized
*/
int dbgio_init(void);

/** \brief   Set IRQ usage.

    The dbgio system defaults to polled usage. Some devices may not support IRQ
    mode at all.

    \param  mode            The mode to use
    
    \retval 0               On success
    \retval -1              On error (errno should be set as appropriate)
*/
int dbgio_set_irq_usage(dbgio_mode_t mode);


/** \brief   Read one character from the console.

    \retval 0               On success
    \retval -1              On error (errno should be set as appropriate)
*/
int dbgio_read(void);

/** \brief   Write one character to the console.

    \note                   Interfaces may require a call to flush() before the
                            output is actually flushed to the console.

    \param  c               The character to write
    
    \retval 1               On success (number of characters written)
    \retval -1              On error (errno should be set as appropriate)
*/
int dbgio_write(int c);

/** \brief   Flush any queued output.

    \retval 0               On success
    \retval -1              On error (errno should be set as appropriate)
*/
int dbgio_flush(void);

/** \brief   Write an entire buffer of data to the console.

    \param  data            The buffer to write
    \param  len             The length of the buffer
    
    \return                 Number of characters written on success, or -1 on
                            failure (errno should be set as appropriate)
*/
int dbgio_write_buffer(const uint8_t *data, size_t len);

/** \brief   Read an entire buffer of data from the console.

    \param  data            The buffer to read into
    \param  len             The length of the buffer
    
    \return                 Number of characters read on success, or -1 on
                            failure (errno should be set as appropriate)
*/
int dbgio_read_buffer(uint8_t *data, size_t len);

/** \brief   Write an entire buffer of data to the console (potentially with
             newline transformations).

    \param  data            The buffer to write
    \param  len             The length of the buffer
    
    \return                 Number of characters written on success, or -1 on
                            failure (errno should be set as appropriate)
*/
int dbgio_write_buffer_xlat(const uint8_t *data, size_t len);

/** \brief   Write a NUL-terminated string to the console.

    \param  str             The string to write
    
    \return                 Number of characters written on success, or -1 on
                            failure (errno should be set as appropriate)
*/
int dbgio_write_str(const char *str);

/** \brief   Disable debug I/O globally.
*/
void dbgio_disable(void);

/** \brief   Enable debug I/O globally. 
*/
void dbgio_enable(void);

/** \brief   Built-in debug I/O printf function.
    
    \param  fmt             A printf() style format string
    \param  ...             Format arguments
    
    \return                 The number of bytes written, or <0 on error (errno
                            should be set as appropriate)
*/
int dbgio_printf(const char *fmt, ...) __printflike(1, 2);

int dbgio_vprintf(const char *fmt, va_list var_args); 

/** @} */

__END_DECLS

#endif  /* __KOS_DBGIO_H */

