/* KallistiOS ##version##

   dcload.c

   Copyright (C) 2025 Donald Haase

*/

#include <stdio.h>

#include <dc/fifo.h>
#include <dc/dcload.h>
#include <arch/irq.h>
#include <dc/memory.h>

/* This is the address where the function pointer for the dcload syscall is fetched from */
#define VEC_DCLOAD        (MEM_AREA_P1_BASE | 0x0C004008)

/*
    This is the single syscall dcload provides. It is then multiplexed out based on the `cmd`
    parameter.
*/

static int dcload_syscall(dcload_cmd_t cmd, void *param1, void *param2, void *param3) {
    uintptr_t *syscall_ptr = (uintptr_t *)VEC_DCLOAD;
    int (*syscall)() = (int (*)())(*syscall_ptr);

    /* Disable IRQs until the syscall returns */
    irq_disable_scoped();

    /* Ensure that the FIFO buffer is clear */
    /* XXX - Is this needed? It seems like something only for serial. */
    while(FIFO_STATUS & FIFO_SH4)            ;

    /* Make the call */
    return syscall(cmd, param1, param2, param3);
}

ssize_t dcload_read(uint32_t hnd, uint8_t *data, size_t len) {
    return dcload_syscall(DCLOAD_READ, (void *)hnd, (void *)data, (void *)len);
}

ssize_t dcload_write(uint32_t hnd, const uint8_t *data, size_t len) {
    return dcload_syscall(DCLOAD_WRITE, (void *)hnd, (void *)data, (void *)len);
}

int dcload_open(const char *fn, int oflags, int mode) {
    return dcload_syscall(DCLOAD_OPEN, (void *)fn, (void *)oflags, (void *)mode);
}

int dcload_close(uint32_t hnd) {
    return dcload_syscall(DCLOAD_CLOSE, (void *)hnd, NULL, NULL);
}

int dcload_creat(const char *path, mode_t mode) {
    return dcload_syscall(DCLOAD_CREAT, (void *)path, (void *)mode, NULL);
}

int dcload_link(const char *fn1, const char *fn2) {
    return dcload_syscall(DCLOAD_LINK, (void *)fn1, (void *)fn2, NULL);
}

int dcload_unlink(const char *fn) {
    return dcload_syscall(DCLOAD_UNLINK, (void *)fn, NULL, NULL);
}

int dcload_chdir(const char *path) {
    return dcload_syscall(DCLOAD_CHDIR, (void *)path, NULL, NULL);
}

int dcload_chmod(const char *path, mode_t mode) {
    return dcload_syscall(DCLOAD_CHMOD, (void *)path, (void *)mode, NULL);
}

off_t dcload_lseek(uint32_t hnd, off_t offset, int whence) {
    return (off_t)dcload_syscall(DCLOAD_LSEEK, (void *)hnd, (void *)offset, (void *)whence);
}

int dcload_fstat(int fildes, dcload_stat_t *buf) {
    return dcload_syscall(DCLOAD_FSTAT, (void *)fildes, (void *)buf, NULL);
}

time_t dcload_time(void) {
    return (time_t)dcload_syscall(DCLOAD_TIME, NULL, NULL, NULL);
}

int dcload_stat(const char *restrict path, dcload_stat_t *restrict buf) {
    return dcload_syscall(DCLOAD_STAT, (void *)path, (void *)buf, NULL);
}

/* Leaving this disabled for now as dcload was written when these values would
    have been 32bit but they are now each 64 bits so they can't be sent
    transparently.
*/
/*
int dcload_utime(const char *path, const struct utimbuf *times) {
    return dcload_syscall(DCLOAD_UTIME, (void *)path,
        (void *) (times ? times->actime : 0), (void *) (times ? times->modtime : 0));
}
*/

int dcload_assignwrkmem(int *buf) {
    return dcload_syscall(DCLOAD_ASSIGNWRKMEM, (void *)buf, NULL, NULL);
}

void dcload_exit(void) {
    dcload_syscall(DCLOAD_EXIT, NULL, NULL, NULL);
}

int dcload_opendir(const char *fn) {
    return dcload_syscall(DCLOAD_OPENDIR, (void *)fn, NULL, NULL);
}

int dcload_closedir(uint32_t hnd) {
    return dcload_syscall(DCLOAD_CLOSEDIR, (void *)hnd, NULL, NULL);
}

struct dirent *dcload_readdir(uint32_t hnd) {
    return (struct dirent *)dcload_syscall(DCLOAD_READDIR, (void *)hnd, NULL, NULL);
}

size_t dcload_gdbpacket(const char* in_buf, size_t in_size, char* out_buf, size_t out_size) {
    /* we have to pack the sizes together because the dcloadsyscall handler
       can only take 4 parameters */
    return dcload_syscall(DCLOAD_GDBPACKET, (void *)in_buf, (void *)((in_size << 16) | (out_size & 0xffff)), (void *)out_buf);
}

uint32_t dcload_gethostinfo(uint32_t *ip, uint32_t *port) {
    return dcload_syscall(DCLOAD_GETHOSTINFO, (void *)ip, (void *)port, NULL);
}

int dcload_rewinddir(uint32_t hnd) {
    return dcload_syscall(DCLOAD_REWINDDIR, (void *)hnd, NULL, NULL);
}
