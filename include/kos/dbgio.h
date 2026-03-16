/* KallistiOS ##version##

   kos/include/dbgio.h
   Copyright (C) 2000, 2004 Megan Potter
   Copyright (C) 2026 Eric Fradella

*/

/** \file    kos/dbgio.h
    \brief   Debug I/O.
    \ingroup logging

    This file contains the Debug I/O system, which abstracts things so that
    various types of debugging tools can be used by programs in KOS. Included
    among these tools is the dcload console (dcload-serial, dcload-ip, and
    fs_dclsocket), a raw serial console, and a framebuffer based console.

    \author Megan Potter
*/

#ifndef __KOS_DBGIO_H
#define __KOS_DBGIO_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>
#include <stdint.h>
#include <sys/queue.h>

/** \brief   Debug I/O Interface.
    \ingroup logging

    This struct represents a single dbgio interface. This should represent
    a generic pollable console interface. We store an ordered, singly-linked
    list of these and fall back from one to the next until one returns true
    for detected() and init(). Users may create and add their own dbgio
    interfaces using the dbgio_add_handler() function.

    \headerfile kos/dbgio.h
*/
typedef struct dbgio_handler {
    /** \brief  Name of the dbgio handler */
    const char  *name;

    /** \brief  Return if the handler is available for auto-selection.

        If not present, will presume unavailable.

        \retval 1           The device is available for auto-selection
        \retval 0           The device is unavailable for auto-selection
    */
    int (*detected)(void);

    /** \brief  Initialize this debug interface with default parameters.

        If not present, will simply succeed. Only needed if it's possible
        to fail at initializing the dbgio device.

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
        \param  mode        1 for IRQ-based usage, 0 for polled I/O
        \retval 0           On success
        \retval -1          On failure
    */
    int (*set_irq_usage)(int mode);

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
    int (*write_buffer)(const uint8_t *data, int len, int xlat);

    /** \brief  Read an entire buffer of data from the console.
        \param  data        The buffer to read into
        \param  len         The length of the buffer
        \return             Number of characters read on success, or -1 on
                            failure (set errno as appropriate)
    */
    int (*read_buffer)(uint8_t *data, int len);

    /** \brief Output even when not selected. */
    bool    output;

    /** \brief dbgio handler list handle.

        Contrary to what doxygen might think, this is not a function.
    */
    SLIST_ENTRY(dbgio_handler) entry;
} dbgio_handler_t;

/** \brief   Add a new dbgio handler to the list.
    \ingroup logging

    This function adds a new dbgio handler to the top of the list. Regardless
    of return value, the handler will be installed.

    \retval 0               No errors.
    \retval Non-zero        The return of a handler's init if set to output by default.
*/
int dbgio_add_handler(dbgio_handler_t *handler);

/** \brief   Remove a dbgio handler from the list.
    \ingroup logging

    This function removes a dbgio handler from the list. If the removed handler
    was the currently selected dbgio handler, the first valid dbgio interface
    in the list will then be selected.

    \retval 0               On success
    \retval -1              On error
*/
int dbgio_remove_handler(dbgio_handler_t *handler);

/** \brief   Select a new dbgio interface by name.
    \ingroup logging

    This function manually selects a new dbgio interface by name. This function
    will allow you to select a device, even if it is not detected.

    \param  name            The dbgio interface to select
    \retval 0               On success

    \retval -1              On error

    \par    Error Conditions:
    \em     ENODEV - The specified device could not be initialized or wasn't found.
*/
int dbgio_dev_select(const char *name);

/** \brief   Select a valid dbgio interface automatically.
    \ingroup logging

    This function selects the first detected dbgio interface in the list.

    \retval 0               On success

    \retval -1              On error

    \par    Error Conditions:
    \em     ENODEV - No devices could be detected/initialized
*/
int dbgio_dev_select_auto(void);

/** \brief   Fetch the name of the currently selected dbgio interface.
    \ingroup logging

    \return                 The name of the current dbgio interface (or NULL if
                            no device is selected)
*/
const char *dbgio_dev_get(void);

/** \brief   Set a dbgio interface to output mode by name.
    \ingroup logging

    This function sets the output status of a named dbgio interface.
    The default operation is for this to be set to false.
    When output is true, the interface will receive all outputs:
    write, write_buffer, and flush regardless of which interface is
    selected.

    If the interface hasn't been initialized and is being set to true, it
    will be initialized. Similarly if setting to false and the interface
    isn't already in use as the selected one it will be shut down.

    \param  name            The dbgio interface to set output.
    \param  output          Whether to enable or disable output mode.

    \retval 0               On success
    \retval -1              On error

    \par    Error Conditions:
    \em     ENODEV - The specified device could not be initialized or wasn't found.
*/
int dbgio_dev_output(const char *name, bool output);

/** \brief   Initialize the dbgio console.
    \ingroup logging

    This function is called internally, and shouldn't need to be called by any
    user programs.

    \retval 0               On success

    \retval -1              On error

    \par    Error Conditions:
    \em     ENODEV - No devices could be detected/initialized
*/
int dbgio_init(void);

/** \brief   Set IRQ usage.
    \ingroup logging

    The dbgio system defaults to polled usage. Some devices may not support IRQ
    mode at all.

    \param  mode            The mode to use

    \retval 0               On success
    \retval -1              On error (errno should be set as appropriate)
*/
int dbgio_set_irq_usage(int mode);

/** \brief   Polled I/O mode.
    \ingroup logging

    \see    dbgio_set_irq_usage()
*/
#define DBGIO_MODE_POLLED 0

/** \brief   IRQ-based I/O mode.
    \ingroup logging

    \see    dbgio_set_irq_usage()
*/
#define DBGIO_MODE_IRQ 1

/** \brief   Read one character from the console.
    \ingroup logging

    \retval 0               On success
    \retval -1              On error (errno should be set as appropriate)
*/
int dbgio_read(void);

/** \brief   Write one character to the console.
    \ingroup logging

    \note                   Interfaces may require a call to flush() before the
                            output is actually flushed to the console.

    \param  c               The character to write

    \retval 1               On success (number of characters written)
    \retval -1              On error (errno should be set as appropriate)
*/
int dbgio_write(int c);

/** \brief   Flush any queued output.
    \ingroup logging

    \retval 0               On success
    \retval -1              On error (errno should be set as appropriate)
*/
int dbgio_flush(void);

/** \brief   Write an entire buffer of data to the console.
    \ingroup logging

    \param  data            The buffer to write
    \param  len             The length of the buffer

    \return                 Number of characters written on success, or -1 on
                            failure (errno should be set as appropriate)
*/
int dbgio_write_buffer(const uint8_t *data, int len);

/** \brief   Read an entire buffer of data from the console.
    \ingroup logging

    \param  data            The buffer to read into
    \param  len             The length of the buffer

    \return                 Number of characters read on success, or -1 on
                            failure (errno should be set as appropriate)
*/
int dbgio_read_buffer(uint8_t *data, int len);

/** \brief   Write an entire buffer of data to the console (potentially with
             newline transformations).
    \ingroup logging

    \param  data            The buffer to write
    \param  len             The length of the buffer

    \return                 Number of characters written on success, or -1 on
                            failure (errno should be set as appropriate)
*/
int dbgio_write_buffer_xlat(const uint8_t *data, int len);

/** \brief   Write a NUL-terminated string to the console.
    \ingroup logging

    \param  str             The string to write

    \return                 Number of characters written on success, or -1 on
                            failure (errno should be set as appropriate)
*/
int dbgio_write_str(const char *str);

/** \brief   Disable debug I/O globally.
    \ingroup logging
*/
void dbgio_disable(void);

/** \brief   Enable debug I/O globally.
    \ingroup logging
*/
void dbgio_enable(void);

/** \brief   Built-in debug I/O printf function.
    \ingroup logging

    \param  fmt             A printf() style format string
    \param  ...             Format arguments

    \return                 The number of bytes written, or <0 on error (errno
                            should be set as appropriate)
*/
int dbgio_printf(const char *fmt, ...) __printflike(1, 2);

__END_DECLS

#endif  /* __KOS_DBGIO_H */

