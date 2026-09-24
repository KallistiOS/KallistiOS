/* KallistiOS ##version##

   nmmgr-lifecycle-test.c
   Copyright (C) 2026 Joseph Black
*/

#include <kos/nmmgr.h>
#include <kos/exports.h>

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <arch/timer.h>

static int failures;
int test_spurious_wakes;
bool test_in_irq;

/* Unsorted tables expose address-underflow mistakes independently of list
   order. High addresses must never wrap into a false preceding symbol. */
export_sym_t kernel_symtab[] = {
    { "high", UINTPTR_MAX - 15 }, { "low", 0x100 }, { NULL, 0 }
};
export_sym_t arch_symtab[] = {
    { "middle", 0x200 }, { NULL, 0 }
};
export_sym_t subarch_symtab[] = { { NULL, 0 } };

#define CHECK(condition) do { \
    if(!(condition)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
        ++failures; \
    } \
} while(0)

typedef struct remove_context {
    nmmgr_handler_t *handler;
    atomic_int result;
    int error;
} remove_context_t;

static nmmgr_handler_t make_handler(const char *path, uint32_t flags,
                                    uint32_t type) {
    nmmgr_handler_t handler = {
        .pid = 0,
        .version = UINT32_C(0x00010000),
        .flags = flags,
        .type = type,
        .list_ent = NMMGR_LIST_INIT,
    };

    strcpy(handler.pathname, path);
    return handler;
}

static void sleep_milliseconds(long milliseconds) {
    struct timespec delay = {
        .tv_sec = milliseconds / 1000,
        .tv_nsec = milliseconds % 1000 * 1000000L,
    };

    nanosleep(&delay, NULL);
}

static void *remove_thread(void *argument) {
    remove_context_t *context = argument;

    int result = nmmgr_handler_remove(context->handler);
    context->error = errno;
    atomic_store(&context->result, result);
    return NULL;
}

static void test_add_lookup_and_snapshot(void) {
    nmmgr_handler_t root = make_handler("/test", 0, NMMGR_TYPE_VFS);
    nmmgr_handler_t device = make_handler("/dev/test", NMMGR_FLAGS_INDEV,
                                          NMMGR_TYPE_SINGLETON);
    nmmgr_handler_t duplicate = make_handler("/TEST", 0,
                                             NMMGR_TYPE_VFS);
    char path[NAME_MAX];
    nmmgr_handler_t *reference;

    CHECK(nmmgr_handler_add(&root) == 0);
    CHECK(nmmgr_handler_add(&device) == 0);
    errno = 0;
    CHECK(nmmgr_handler_add(&duplicate) < 0 && errno == EEXIST);

    reference = nmmgr_lookup_ref("/test/file");
    CHECK(reference == &root);
    CHECK(nmmgr_handler_release(reference) == 0);

    CHECK(nmmgr_handler_get_path(0, NMMGR_TYPE_SINGLETON,
                                 NMMGR_FLAGS_INDEV, 0, path,
                                 sizeof(path)) == 0);
    CHECK(!strcmp(path, "/dev/test"));
    CHECK(nmmgr_handler_remove(&device) == 0);
    CHECK(nmmgr_handler_remove(&root) == 0);
}

static void test_timeout_unpublishes(void) {
    nmmgr_handler_t handler = make_handler("/timeout", 0, NMMGR_TYPE_VFS);
    nmmgr_handler_t *reference;

    CHECK(nmmgr_handler_add(&handler) == 0);
    reference = nmmgr_lookup_ref("/timeout/file");
    CHECK(reference == &handler);

    errno = 0;
    CHECK(nmmgr_handler_remove_timed(&handler, 20) < 0);
    CHECK(errno == ETIMEDOUT);
    errno = 0;
    CHECK(nmmgr_lookup_ref("/timeout/file") == NULL && errno == ENOENT);

    CHECK(nmmgr_handler_release(reference) == 0);
    CHECK(nmmgr_handler_remove(&handler) == 0);
}

static void test_blocking_drain(void) {
    nmmgr_handler_t handler = make_handler("/drain", 0, NMMGR_TYPE_VFS);
    remove_context_t context = {
        .handler = &handler,
        .result = -2,
    };
    nmmgr_handler_t *reference;
    pthread_t thread;

    CHECK(nmmgr_handler_add(&handler) == 0);
    reference = nmmgr_lookup_ref("/drain/file");
    CHECK(reference == &handler);
    CHECK(pthread_create(&thread, NULL, remove_thread, &context) == 0);
    for(unsigned i = 0; i < 1000; ++i) {
        nmmgr_handler_t *probe = nmmgr_lookup_ref("/drain/file");
        if(!probe)
            break;
        CHECK(nmmgr_handler_release(probe) == 0);
        sleep_milliseconds(1);
    }

    errno = 0;
    CHECK(nmmgr_lookup_ref("/drain/file") == NULL && errno == ENOENT);
    CHECK(context.result == -2);
    CHECK(nmmgr_handler_release(reference) == 0);
    CHECK(pthread_join(thread, NULL) == 0);
    CHECK(context.result == 0);
}

static void test_alias_retains_target(void) {
    nmmgr_handler_t target = make_handler("/target", 0, NMMGR_TYPE_VFS);
    alias_handler_t alias = {
        .nmmgr = {
            .pathname = "/alias",
            .pid = 0,
            .version = UINT32_C(0x00010000),
            .flags = NMMGR_FLAGS_ALIAS,
            .type = NMMGR_TYPE_VFS,
            .list_ent = NMMGR_LIST_INIT,
        },
        .alias = &target,
    };
    nmmgr_handler_t *reference;

    CHECK(nmmgr_handler_add(&target) == 0);
    CHECK(nmmgr_handler_add(&alias.nmmgr) == 0);
    reference = nmmgr_lookup_ref("/alias/file");
    CHECK(reference == &target);
    CHECK(nmmgr_handler_remove(&alias.nmmgr) == 0);
    CHECK(nmmgr_handler_release(reference) == 0);
    CHECK(nmmgr_handler_remove(&target) == 0);
}

static void test_export_lookup(void) {
    test_in_irq = true;
    CHECK(export_lookup_addr(0xff) == NULL);
    CHECK(export_lookup_addr(0x280) == &arch_symtab[0]);
    CHECK(export_lookup_addr(UINTPTR_MAX) == &kernel_symtab[0]);
    test_in_irq = false;
    nmmgr_handler_t wrong_type = make_handler("/not-a-symtab", 0,
                                             NMMGR_TYPE_VFS);
    CHECK(nmmgr_handler_add(&wrong_type) == 0);
    CHECK(export_lookup_path("bad", "/not-a-symtab") == NULL);
    CHECK(nmmgr_handler_remove_timed(&wrong_type, 1) == 0);
    export_init();
    CHECK(export_lookup_addr(0) == NULL);
    CHECK(export_lookup_addr(0xff) == NULL);
    CHECK(export_lookup_addr(0x100) == &kernel_symtab[1]);
    CHECK(export_lookup_addr(0x1ff) == &kernel_symtab[1]);
    CHECK(export_lookup_addr(0x200) == &arch_symtab[0]);
    CHECK(export_lookup_addr(0x280) == &arch_symtab[0]);
    CHECK(export_lookup_addr(UINTPTR_MAX) == &kernel_symtab[0]);
    CHECK(export_lookup("low") == &kernel_symtab[1]);
    CHECK(export_lookup_path("middle", "sym/kernel/arch") == &arch_symtab[0]);
    CHECK(export_lookup("absent") == NULL);
    /* Every lookup must release its temporary handler reference. Otherwise
       shutdown/removal would hang even with no concurrent table unload. */
    const char *paths[] = {
        "sym/kernel/kernel", "sym/kernel/arch", "sym/kernel/subarch"
    };
    for(size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        nmmgr_handler_t *handler = nmmgr_lookup(paths[i]);
        CHECK(handler != NULL);
        if(handler)
            CHECK(nmmgr_handler_remove_timed(handler, 1) == 0);
    }
}

static void test_spurious_deadline(void) {
    nmmgr_handler_t handler = make_handler("/spurious", 0, NMMGR_TYPE_VFS);
    CHECK(nmmgr_handler_add(&handler) == 0);
    CHECK(nmmgr_handler_retain(&handler) == 0);
    test_spurious_wakes = 100;
    CHECK(nmmgr_handler_remove_timed(&handler, 10) < 0);
    CHECK(errno == ETIMEDOUT);
    /* Each injected wake takes >=1 ms. A reset-on-every-wake implementation
       consumes all 100 before timing out; an absolute deadline does not. */
    CHECK(test_spurious_wakes > 0);
    test_spurious_wakes = 0;
    CHECK(nmmgr_handler_retain(&handler) < 0 && errno == ENOENT);
    CHECK(nmmgr_handler_release(&handler) == 0);
    CHECK(nmmgr_handler_remove(&handler) == 0);
    CHECK(nmmgr_handler_release(&handler) < 0 && errno == EINVAL);
}

int main(void) {
    nmmgr_init();
    test_add_lookup_and_snapshot();
    test_timeout_unpublishes();
    test_blocking_drain();
    test_alias_retains_target();
    test_export_lookup();
    test_spurious_deadline();
    nmmgr_shutdown();

    if(failures) {
        fprintf(stderr, "%d name-manager lifecycle checks failed\n", failures);
        return 1;
    }

    puts("name-manager lifecycle tests passed");
    return 0;
}
