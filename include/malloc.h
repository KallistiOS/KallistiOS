/* KallistiOS ##version##

   malloc.h
   Copyright (C) 2024 Paul Cercueil
*/

/* <malloc.h> shadows Newlib's own header.
   Since we do not use Newlib's memory allocator, KallistiOS must not include
   <malloc.h>. */
#error <malloc.h> must not be included.
