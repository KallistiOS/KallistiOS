/* KallistiOS ##version##

   kos/md5.h
   Copyright (C) 2010 Lawrence Sebald
*/

#ifndef __KOS_MD5_H
#define __KOS_MD5_H

/** \file   kos/md5.h
    \brief  Message Digest 5 (MD5) hashing support.

    This file provides the functionality to compute MD5 hashes over any data
    buffer. While MD5 isn't considered a safe cryptographic hash any more, it
    still has its uses.

    \author Lawrence Sebald
*/

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

/** \brief  MD5 context.

    This structure contains the variables needed to maintain the internal state
    of the MD5 code. You should not manipulate these variables manually, but
    rather use the kos_md5_* functions to do everything you need.

    \headerfile kos/md5.h
*/
typedef struct kos_md5_cxt {
    uint64_t size;        /**< \brief Size of the data in buf. */
    uint32_t hash[4];     /**< \brief Intermediate hash value. */
    uint8_t  buf[64];     /**< \brief Temporary storage of values to be hashed. */
} kos_md5_cxt_t;

/** \brief  Initialize a MD5 context.

    This function initializes the context passed in to the initial state needed
    for computing a MD5 hash. You must call this function to initialize the
    state variables before attempting to hash any blocks of data.

    \param  cxt         The MD5 context to initialize.
*/
void kos_md5_start(kos_md5_cxt_t *cxt);

/** \brief  Hash a block of data with MD5.

    This function is used to hash the block of data input into the function with
    MD5, updating the state context as appropriate. If the data does not fill an
    entire block of 64-bytes (or there is left-over data), it will be stored in
    the context for hashing with a future block. Thus, do not attempt to read
    the intermediate hash value, as it will not be complete.

    \param  cxt         The MD5 context to use.
    \param  input       The block of data to hash.
    \param  size        The number of bytes of input data passed in.
*/
void kos_md5_hash_block(kos_md5_cxt_t *cxt, const uint8_t *input, uint32_t size);

/** \brief  Complete a MD5 hash.

    This function computes the final MD5 hash of the context passed in,
    returning the completed digest in the output parameter.

    \param  cxt         The MD5 context to finalize.
    \param  output      Where to store the final digest.
*/
void kos_md5_finish(kos_md5_cxt_t *cxt, uint8_t output[16]);

/** \brief  Compute the hash of a block of data with MD5.

    This function is used to hash a full block of data without messing around
    with any contexts or anything else of the sort. This is appropriate if you
    have all the data you want to hash readily available. It takes care of all
    of the context setup and teardown for you.

    \param  input       The data to hash.
    \param  size        The number of bytes of input data passed in.
    \param  output      Where to store the final message digest.
*/
void kos_md5(const uint8_t *input, uint32_t size, uint8_t output[16]);

__END_DECLS

#endif /* !__KOS_MD5_H */
