/* KallistiOS ##version##

   kernel/kosload.c
   Copyright (C) 2002 Andrew Kieschnick
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2012 Lawrence Sebald
   Copyright (C) 2025 Donald Haase
   Copyright (C) 2026 Andy Barajas

*/

/*
   Low-level driver for the dc-tool host loader.

   Provides the single multiplexed loader syscall and typed command wrappers,
   connection detection, and the GDB-over-loader transport.  The dc-tool host
   filesystem VFS and console (kernel/fs/fs_kosload.c) are layered on top of
   this driver.

   All platform-specific bits (addresses, FIFO flush) live in the per-arch
   <arch/kosload.h> header; this file is fully portable.
*/

/* Platform-specific addresses (KOSLOAD_SYSCALL_ADDR, KOSLOAD_MAGIC_ADDR,
   kosload_flush_fifo) come from the arch header. */
#include <arch/kosload.h>

#include <kos/kosload.h>
#include <kos/irq.h>

#include <stdint.h>
#include <dirent.h>

/* Same 22-command enum used on every platform. */
typedef enum {
    KOSLOAD_READ         = 0,
    KOSLOAD_WRITE        = 1,
    KOSLOAD_OPEN         = 2,
    KOSLOAD_CLOSE        = 3,
    KOSLOAD_CREAT        = 4,
    KOSLOAD_LINK         = 5,
    KOSLOAD_UNLINK       = 6,
    KOSLOAD_CHDIR        = 7,
    KOSLOAD_CHMOD        = 8,
    KOSLOAD_LSEEK        = 9,
    KOSLOAD_FSTAT        = 10,
    KOSLOAD_TIME         = 11,
    KOSLOAD_STAT         = 12,
    KOSLOAD_UTIME        = 13,
    KOSLOAD_ASSIGNWRKMEM = 14,
    KOSLOAD_EXIT         = 15,
    KOSLOAD_OPENDIR      = 16,
    KOSLOAD_CLOSEDIR     = 17,
    KOSLOAD_READDIR      = 18,
    KOSLOAD_GETHOSTINFO  = 19,
    KOSLOAD_GDBPACKET    = 20,
    KOSLOAD_REWINDDIR    = 21
} kosload_cmd_t;

/*
    This is the single syscall kosload provides. It is then multiplexed out based on the `cmd`
    parameter.
*/
static int kosload_syscall(kosload_cmd_t cmd, void *param1, void *param2, void *param3) {
    uintptr_t *syscall_ptr = (uintptr_t *)KOSLOAD_SYSCALL_ADDR;
    int (*syscall)() = (int (*)())(*syscall_ptr);

    /* Disable IRQs until the syscall returns */
    irq_disable_scoped();

    /* Ensure that the FIFO buffer is clear */
    /* XXX - Is this needed? It seems like something only for serial. */
    kosload_flush_fifo();

    /* Make the call */
    return syscall(cmd, param1, param2, param3);
}

/* ---- Low-level command wrappers ------------------------------------------ */

ssize_t kosload_read(uint32_t hnd, uint8_t *data, size_t len) {
    return kosload_syscall(KOSLOAD_READ, (void *)hnd, (void *)data, (void *)len);
}

ssize_t kosload_write(uint32_t hnd, const uint8_t *data, size_t len) {
    return kosload_syscall(KOSLOAD_WRITE, (void *)hnd, (void *)data, (void *)len);
}

int kosload_open(const char *fn, int oflags, int mode) {
    return kosload_syscall(KOSLOAD_OPEN, (void *)fn, (void *)oflags, (void *)mode);
}

int kosload_close(uint32_t hnd) {
    return kosload_syscall(KOSLOAD_CLOSE, (void *)hnd, NULL, NULL);
}

int kosload_creat(const char *path, mode_t mode) {
    return kosload_syscall(KOSLOAD_CREAT, (void *)path, (void *)mode, NULL);
}

int kosload_link(const char *fn1, const char *fn2) {
    return kosload_syscall(KOSLOAD_LINK, (void *)fn1, (void *)fn2, NULL);
}

int kosload_unlink(const char *fn) {
    return kosload_syscall(KOSLOAD_UNLINK, (void *)fn, NULL, NULL);
}

int kosload_chdir(const char *path) {
    return kosload_syscall(KOSLOAD_CHDIR, (void *)path, NULL, NULL);
}

int kosload_chmod(const char *path, mode_t mode) {
    return kosload_syscall(KOSLOAD_CHMOD, (void *)path, (void *)mode, NULL);
}

off_t kosload_lseek(uint32_t hnd, off_t offset, int whence) {
    return (off_t)kosload_syscall(KOSLOAD_LSEEK,
                                  (void *)hnd, (void *)offset, (void *)whence);
}

int kosload_fstat(int fildes, kosload_stat_t *buf) {
    return kosload_syscall(KOSLOAD_FSTAT, (void *)fildes, (void *)buf, NULL);
}

time_t kosload_time(void) {
    return (time_t)kosload_syscall(KOSLOAD_TIME, NULL, NULL, NULL);
}

int kosload_stat(const char *restrict path, kosload_stat_t *restrict buf) {
    return kosload_syscall(KOSLOAD_STAT, (void *)path, (void *)buf, NULL);
}

/* Leaving this disabled for now as kosload was written when these values would
    have been 32bit but they are now each 64 bits so they can't be sent
    transparently.
*/
/*
int kosload_utime(const char *path, const struct utimbuf *times) {
    return kosload_syscall(KOSLOAD_UTIME, (void *)path,
        (void *) (times ? times->actime : 0), (void *) (times ? times->modtime : 0));
}
*/

int kosload_assignwrkmem(int *buf) {
    return kosload_syscall(KOSLOAD_ASSIGNWRKMEM, (void *)buf, NULL, NULL);
}

static __attribute__((unused)) void kosload_exit(void) {
    kosload_syscall(KOSLOAD_EXIT, NULL, NULL, NULL);
}

int kosload_opendir(const char *fn) {
    return kosload_syscall(KOSLOAD_OPENDIR, (void *)fn, NULL, NULL);
}

int kosload_closedir(uint32_t hnd) {
    return kosload_syscall(KOSLOAD_CLOSEDIR, (void *)hnd, NULL, NULL);
}

struct dirent *kosload_readdir(uint32_t hnd) {
    return (struct dirent *)kosload_syscall(KOSLOAD_READDIR, (void *)hnd, NULL, NULL);
}

uint32_t kosload_gethostinfo(uint32_t *ip, uint32_t *port) {
    return (uint32_t)kosload_syscall(KOSLOAD_GETHOSTINFO,
                                     (void *)ip, (void *)port, NULL);
}

size_t kosload_gdbpacket(const char *in_buf, size_t in_size,
                         char *out_buf, size_t out_size) {
    /* we have to pack the sizes together because the kosloadsyscall handler
       can only take 4 parameters */
    return (size_t)kosload_syscall(KOSLOAD_GDBPACKET,
                                   (void *)in_buf,
                                   (void *)((in_size << 16) | (out_size & 0xffff)),
                                   (void *)out_buf);
}

int kosload_rewinddir(uint32_t hnd) {
    return kosload_syscall(KOSLOAD_REWINDDIR, (void *)hnd, NULL, NULL);
}

/* ---- Detection ----------------------------------------------------------- */

int syscall_kosload_detected(void) {
    if(*(uintptr_t *)KOSLOAD_MAGIC_ADDR == KOSLOAD_CONSOLE_ON)
        return 1;
    else
        return 0;
}
