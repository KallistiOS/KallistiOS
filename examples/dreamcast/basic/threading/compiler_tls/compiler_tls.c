/* KallistiOS ##version##

   compiler_tls.c

   (c)2023 Colton Pawielski

   A simple example showing off thread local variables

   This example launches two threads that access variables
   placed in the TLS segment by the compiler. The compiler
   is then able to generate trivial lookups based on the GBR
   register which holds the address to the current thread's
   control block.

 */

#include <kos.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>

#if (__GNUC__ <= 4)
/* GCC4 only supports using TLS with the __thread identifier,
   even when passed the -std=c99 flag */
#define thread_local __thread
#else
/* Newer versions of GCC use C11's _Thread_local to specify TLS */
#define thread_local _Thread_local
#endif

static _Alignas(64) thread_local uint32_t tbss_test = 0;
static _Alignas(128) thread_local char string[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static _Alignas(32) thread_local uint64_t tdata_test = 5;
static volatile int errorCount = 0;

/* Thread Function */
void *thd(void *v) {
    int i;
    int id = (int) v;

    printf("Started Thread %d\n", id);

    for (i = 0; i < 5; i++){        
        printf("Thread[%d]\ttbss_test = 0x%lX\n", id, tbss_test);
        tbss_test++;
        thd_sleep(50);
    }

    if(tbss_test != 5) {
        printf("Thread[%d]\ttbss_test: %s\n", id, "FAILURE");
        ++errorCount;
    }

    for (i = 0; i < 5; i++){
        printf("Thread[%d]\ttdata_test = 0x%llX\n", id, tdata_test);
        tdata_test++;
        thd_sleep(50);
    }

    if(tdata_test != 10) {
        printf("Thread[%d]\ttdata_test: %s\n", id, "FAILURE");
        ++errorCount;  
    }

    if(strcmp(string, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != 0) {
        printf("Incorrectly aligned string data!\n");
        ++errorCount;
    }

    printf("Finished Thread %d\n", id);
    return NULL;
}

/* The main program */
int main(int argc, char **argv) {
    const int thread_count = 5;

    int i;
    kthread_t * threads[thread_count];   

    printf("Starting Threads\n");
    for(i = 0; i < thread_count; i++) {
        threads[i] = thd_create(0, thd, (void *)i);
    };

    for (i = 0; i < 5; i++) {        
        printf("Thread[%s]\ttbss_test = 0x%lX\n", "main", tbss_test);
        tbss_test++;
        thd_sleep(50);
    }

    if(tbss_test != 5) {
        printf("Thread[%s]\ttbss_test: %s\n", "main", "FAILURE");
        ++errorCount;
    }

    for (i = 0; i < 5; i++) {
        printf("Thread[%s]\ttdata_test = 0x%llX\n", "main", tdata_test);
        tdata_test++;
        thd_sleep(50);
    }
    
    if(tdata_test != 10) {
        printf("Thread[%s]\ttdata_test: %s\n", "main", "FAILURE");
        ++errorCount;
    }

    for(i = 0; i < thread_count; i++)
        thd_join(threads[i], NULL);
    
    printf("Threads Finished!\n");
    
    if(!errorCount) {
        printf("SUCCESS!\n");
        return EXIT_SUCCESS;
    } else {
        printf("%d ERRORS!\n", errorCount);
        return EXIT_FAILURE;
    }
}
