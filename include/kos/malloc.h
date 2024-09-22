/* KallistiOS ##version##

   malloc.h
   Copyright (C) 2024 Paul Cercueil
*/

#ifndef __MALLOC_H
#define __MALLOC_H

#include <stdlib.h>

void malloc_stats(void);

int malloc_irq_safe(void);

#endif /* __MALLOC_H */
