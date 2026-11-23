/* Libkosh for KallistiOS ##version##

   kosh.h
   (c)2002 Megan Potter
*/

#ifndef __KOSH_KOSH_H
#define __KOSH_KOSH_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** Call to begin a KOSH thread on the currently open conio. Assumes
    that you've already initiated the libconio system. */
int kosh_init(void);

/** Call to terminate the KOSH thread on the currently open conio. */
void kosh_shutdown(void);

/** Call to add a built-in command to the shell. */
void kosh_builtin_add(const char *cmd, const char *helpmsg, void (*handler)(int argc, char *argv[]));

/** Call to remove an added built-in command from the shell. */
void kosh_builtin_remove(const char *cmd);

/** Call to wait for the KOSH thread to exit (e.g. in a standalone shell) */
void kosh_join(void);

__END_DECLS

#endif  /* __KOSH_KOSH_H */

