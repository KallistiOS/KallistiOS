/* KallistiOS ##version##
   Copyright (C) 2026 Joseph Black
   Public-API descriptor/handler lifetime regression; no disc or network I/O.
*/
#include <kos.h>
#include <errno.h>
#include <stdio.h>
#include <stdatomic.h>
#include <string.h>
#include <stdarg.h>

KOS_INIT_FLAGS(INIT_IRQ | INIT_FS_ALL | INIT_NO_DCLOAD);

static unsigned checks, failures;
/* Keep diagnostics independent of descriptors deliberately closed by tests. */
static void probe_log(const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    dbgio_write_str(buffer);
}
#define CHECK(c) do { ++checks; if(!(c)) { ++failures; \
    probe_log("FS-LIFETIME: FAIL line=%d: %s errno=%d\n", __LINE__, #c, errno); \
} } while(0)

static semaphore_t entered, resume;
static atomic_int closes, removed;
static bool block_read, block_stat;
static int token;
static file_t active_fd;
static int read_errno;
static ssize_t read_result;
static int stat_result;

static void *test_open(vfs_handler_t *handler, const char *path, int mode) {
    (void)handler; (void)mode;
    if(!strcmp(path, "/fail")) { errno = EIO; return NULL; }
    return &token;
}
static int test_close(void *handle) {
    (void)handle;
    atomic_fetch_add(&closes, 1);
    /* A close callback must not overwrite an in-flight operation's errno. */
    errno = EDOM;
    return 0;
}
static ssize_t test_read(void *handle, void *data, size_t count) {
    (void)handle; (void)data; (void)count;
    if(block_read) {
        sem_signal(&entered);
        sem_wait(&resume);
    }
    errno = EIO;
    return -1;
}
static int test_stat(vfs_handler_t *handler, const char *path,
                     struct stat *st, int flag) {
    (void)handler; (void)path; (void)flag;
    if(block_stat) { sem_signal(&entered); sem_wait(&resume); }
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFREG;
    return 0;
}
static vfs_handler_t handler = {
    .nmmgr = { .pathname = "/lifetime", .version = 0x10000,
               .type = NMMGR_TYPE_VFS, .list_ent = NMMGR_LIST_INIT },
    .open = test_open, .close = test_close, .read = test_read, .stat = test_stat,
};
static void *reader(void *arg) {
    (void)arg;
    char byte;
    read_result = fs_read(active_fd, &byte, 1);
    read_errno = errno;
    return NULL;
}
static void *statter(void *arg) {
    (void)arg;
    struct stat st;
    stat_result = fs_stat("/lifetime/file", &st, 0);
    return NULL;
}
static void *remover(void *arg) {
    (void)arg;
    atomic_store(&removed, nmmgr_handler_remove(&handler.nmmgr));
    return NULL;
}
static void *rom_unmount(void *arg) {
    (void)arg;
    atomic_store(&removed, fs_romdisk_unmount("/rd"));
    return NULL;
}
static void await_unpublish(void) {
    for(unsigned i = 0; i < 1000; ++i) {
        nmmgr_handler_t *ref = nmmgr_lookup_ref("/lifetime/file");
        if(!ref) return;
        nmmgr_handler_release(ref);
        thd_sleep(1);
    }
    CHECK(false);
}

int main(void) {
    sem_init(&entered, 0); sem_init(&resume, 0);
    CHECK(nmmgr_handler_add(&handler.nmmgr) == 0);
    CHECK(fs_open("/lifetime/fail", O_RDONLY) == FILEHND_INVALID);
    CHECK(errno == EIO);
    CHECK(nmmgr_handler_remove_timed(&handler.nmmgr, 1) == 0);

    CHECK(nmmgr_handler_add(&handler.nmmgr) == 0);
    file_t fd = fs_open("/lifetime/file", O_RDONLY);
    CHECK(fd != FILEHND_INVALID);
    CHECK(fs_dup2(fd, fd) == fd);
    file_t dup = fs_dup(fd);
    CHECK(dup != FILEHND_INVALID);
    int before = atomic_load(&closes);
    CHECK(fs_close(fd) == 0);
    CHECK(atomic_load(&closes) == before);
    CHECK(fs_close(dup) == 0);
    CHECK(atomic_load(&closes) == before + 1);
    CHECK(nmmgr_handler_remove_timed(&handler.nmmgr, 1) == 0);

    /* Close and unmount while read is parked inside the real VFS call. */
    CHECK(nmmgr_handler_add(&handler.nmmgr) == 0);
    active_fd = fs_open("/lifetime/file", O_RDONLY);
    CHECK(active_fd != FILEHND_INVALID);
    block_read = true;
    before = atomic_load(&closes);
    kthread_t *worker = thd_create(0, reader, NULL);
    CHECK(worker != NULL);
    if(!worker) return 1;
    CHECK(sem_wait_timed(&entered, 1000) == 0);
    CHECK(fs_close(active_fd) == 0);
    CHECK(atomic_load(&closes) == before);
    atomic_store(&removed, -2);
    kthread_t *unmount = thd_create(0, remover, NULL);
    CHECK(unmount != NULL);
    if(!unmount) return 1;
    await_unpublish();
    CHECK(atomic_load(&removed) == -2);
    CHECK(fs_open("/lifetime/file", O_RDONLY) == FILEHND_INVALID);
    sem_signal(&resume);
    CHECK(thd_join(worker, NULL) == 0);
    CHECK(thd_join(unmount, NULL) == 0);
    CHECK(read_result == -1 && read_errno == EIO);
    CHECK(atomic_load(&closes) == before + 1);
    CHECK(atomic_load(&removed) == 0);
    block_read = false;

    /* Path operations also retain the handler across their callback. */
    CHECK(nmmgr_handler_add(&handler.nmmgr) == 0);
    block_stat = true;
    worker = thd_create(0, statter, NULL);
    CHECK(worker != NULL);
    if(!worker) return 1;
    CHECK(sem_wait_timed(&entered, 1000) == 0);
    CHECK(nmmgr_handler_remove_timed(&handler.nmmgr, 10) == -1);
    CHECK(errno == ETIMEDOUT);
    sem_signal(&resume);
    CHECK(thd_join(worker, NULL) == 0);
    CHECK(stat_result == 0);
    CHECK(nmmgr_handler_remove(&handler.nmmgr) == 0);
    block_stat = false;

    /* Real mount: unmount must drop fh_mutex so the final close can drain. */
    fd = fs_open("/rd/test.txt", O_RDONLY);
    CHECK(fd != FILEHND_INVALID);
    if(fd == FILEHND_INVALID) return 1;
    atomic_store(&removed, -2);
    unmount = thd_create(0, rom_unmount, NULL);
    CHECK(unmount != NULL);
    if(!unmount) return 1;
    bool hidden = false;
    for(unsigned i = 0; i < 1000; ++i) {
        nmmgr_handler_t *ref = nmmgr_lookup_ref("/rd/test.txt");
        if(!ref) { hidden = true; break; }
        nmmgr_handler_release(ref);
        thd_sleep(1);
    }
    CHECK(hidden && atomic_load(&removed) == -2);
    char fixture[32] = {0};
    CHECK(fs_read(fd, fixture, sizeof(fixture)) == 17);
    CHECK(!strcmp(fixture, "lifetime fixture\n"));
    CHECK(fs_close(fd) == 0);
    CHECK(thd_join(unmount, NULL) == 0);
    CHECK(atomic_load(&removed) == 0);

    /* Fill descriptors to exercise failure ownership without mocking VFS. */
    CHECK(nmmgr_handler_add(&handler.nmmgr) == 0);
    file_t fds[FD_SETSIZE];
    size_t count = 0;
    while(count < FD_SETSIZE) {
        fd = fs_open_handle(&handler, &token);
        if(fd == FILEHND_INVALID) break;
        fds[count++] = fd;
    }
    CHECK(count > 0 && count < FD_SETSIZE && errno == EMFILE);
    before = atomic_load(&closes);
    CHECK(fs_open_handle(&handler, &token) == FILEHND_INVALID);
    CHECK(errno == EMFILE && atomic_load(&closes) == before);
    CHECK(fs_open("/lifetime/file", O_RDONLY) == FILEHND_INVALID);
    CHECK(errno == EMFILE && atomic_load(&closes) == before + 1);
    for(size_t i = 0; i < count; ++i) CHECK(fs_close(fds[i]) == 0);
    CHECK(nmmgr_handler_remove_timed(&handler.nmmgr, 1) == 0);

    CHECK(nmmgr_handler_add(&handler.nmmgr) == 0);
    fd = fs_open_handle(&handler, &token);
    CHECK(fd != FILEHND_INVALID);
    before = atomic_load(&closes);
    fs_shutdown();
    CHECK(atomic_load(&closes) == before + 1);
    CHECK(fs_open_handle(&handler, &token) == FILEHND_INVALID);
    CHECK(errno == ENODEV && atomic_load(&closes) == before + 1);
    fs_init();
    fd = fs_open("/lifetime/file", O_RDONLY);
    CHECK(fd != FILEHND_INVALID);
    CHECK(fs_close(fd) == 0);
    CHECK(nmmgr_handler_remove_timed(&handler.nmmgr, 1) == 0);
    sem_destroy(&entered); sem_destroy(&resume);
    probe_log("FS-LIFETIME: %s checks=%u\n", failures ? "FAIL" : "PASS", checks);
    return failures ? 1 : 0;
}
