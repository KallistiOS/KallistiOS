/* KallistiOS ##version##

   posix_memalign.c
   Copyright (C) 2023 Falco Girgis
*/

#include <stdlib.h>
#include <errno.h>

static inline int is_power_of_two(size_t x) {
   return (x & (x - 1)) == 0;
}

static inline size_t aligned_size(size_t size, size_t alignment) {
   const size_t align_rem = size % alignment;
   size_t new_size = size;

   if(align_rem)
      new_size += (alignment - align_rem);

   return new_size;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
   if(alignment == 0 || !is_power_of_two(alignment) || alignment % sizeof(void*)) {
      *memptr = NULL;
      return EINVAL; 
   }

   if(!size) {
      *memptr = NULL;
      return 0;
   }

   size = aligned_size(size, alignment);

   printf("ALIGN = %zu, SIZE = %zu\n", alignment, size);

   *memptr = aligned_alloc(alignment, size);

   printf("MEMPTR = %p\n", *memptr);

   return *memptr? 0 : ENOMEM;
}