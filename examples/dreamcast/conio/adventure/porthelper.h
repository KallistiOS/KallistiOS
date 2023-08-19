#ifndef __PORTHELPER_H
#define __PORTHELPER_H

/* This file will ease porting to KOS */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <kos.h>
#include <conio/conio.h>

/* Some KOS macros to help us port */
#ifdef fprintf
#undef fprintf
#endif
#define fprintf(a, fmt, args...) conio_printf(fmt, ## args)
#define printf(fmt, args...) ({ conio_printf(fmt, ## args); })
#define fgets(buf, len, a) ({ conio_input_getline(1, buf, len); 1; })

#define srandom(a) ({ })
#define err(a, b...) ({ })
#define errx(a, b...) ({ })

#undef getchar
#undef putchar
int getchar(void);

#endif  /* __PORTHELPER_H */
