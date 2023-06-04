/**********************************************************************
 *
 * Filename:    memtest.h
 * 
 * Description: Memory-testing module API.
 *
 * Notes:       The memory tests can be easily ported to systems with
 *              different data bus widths by redefining 'datum' type.
 *
 * 
 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#ifndef _memtest_h
#define _memtest_h

#include <stdint.h>

/*
 * Define NULL pointer value.
 */
#ifndef NULL
#define NULL  (void *) 0
#endif

/*
 * Set the data bus width.
 */
typedef uint32_t datum;

/*
 * Function prototypes.
 */
datum   memTestDataBus(volatile datum * address);
datum * memTestAddressBus(volatile datum * baseAddress, unsigned long nBytes);
datum * memTestDevice(volatile datum * baseAddress, unsigned long nBytes);


#endif /* _memtest_h */
