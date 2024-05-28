/* KallistiOS ##version##

   ucs.h
   Copyright (C) 2019 Lawrence Sebald
*/

#ifndef __FAT_UCS_H
#define __FAT_UCS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stddef.h>
#include <stdint.h>

int fat_utf8_to_ucs2(uint16_t *out, const uint8_t *in, size_t olen,
                     size_t ilen);
int fat_ucs2_to_utf8(uint8_t *out, const uint16_t *in, size_t olen,
                     size_t ilen);
size_t fat_strlen_ucs2(const uint16_t *in);
void fat_ucs2_tolower(uint16_t *in, size_t len);

__END_DECLS

#endif /* !__FAT_UCS_H */
