/* KallistiOS ##version##

   compiler_tls.c

   (c) 2023 Colton Pawielski
   (c) 2023 Falco Girgis

   A simple example showing off thread local variables

   This example launches several threads that access variables
   placed in the TLS segment by the compiler. The compiler
   is then able to generate trivial lookups based on the GBR
   register which holds the address to the current threads's
   control block.

   This example also doubles as a validator for KOS and the 
   toolchain's TLS implementation by verifying proper behavior
   of each TLS segment with various different kinds of data 
   and alignment requirements. 

 */

#include <kos.h>
#include <stdbool.h>
#include <stdlib.h>

#if (__GNUC__ <= 4)
/* GCC4 only supports using TLS with the __thread identifier,
   even when passed the -std=c99 flag */
#define thread_local __thread
#else
/* Newer versions of GCC use C11's _Thread_local to specify TLS */
#define thread_local _Thread_local
#endif

typedef struct {
    uint8_t inner[3];
} Align4;

typedef struct {
    uint8_t inner[3];
} Align16;

/* Various types of thread-local variables, coming from both the .TDATA and .TBSS 
   segments with both manual over-alignment and default alignment. 

   NOTE: For the sake of validation, these must be declared volatile, otherwise the 
   compiler will optimize away conditionals checking for values that it knows should be
   constant! 
*/
static thread_local volatile _Alignas(4)  Align4   tls_buff4       = {.inner = {2, 2, 2}};
static thread_local volatile _Alignas(16) Align16  tls_buff16      = {.inner = {1, 1, 1}};
static thread_local volatile              uint16_t tls_uint16[256] = { 0 };
static thread_local volatile _Alignas(32) uint32_t tbss_test       = 0;
static thread_local volatile _Alignas(32) char     tls_string[]    = { "abcdefghijklmnopqrstuvwxyz012345" };
static thread_local volatile              uint32_t tdata_test      = 0x5A;

/* 
   Main thread function. Puts each thread through an array of 
   tests on its own set of thread-local data to ensure proper
   initialization, alignment, and uniqueness.
 */
void *thd(void *v) {
    int i;
    int id = (int)v;
    int ret = 0;

    printf("Started Thread %d\n", id);

    /* Ensure zero-initialized .TBSS data have the correct initial 
       values and are unique to each thread. */
    for (i = 0; i < 5; i++){        
        printf("Thread[%d]\tbss_test = 0x%lX\n", id, tbss_test);
        tbss_test++;
        thd_sleep(50);
    }

    if(tbss_test != 5) {
        fprintf(stderr, "TBSS data check failed!\n");
        ret = -1;
    }

    /* Ensure value-initialized .TDATA data have the correct initial 
       values and are unique to each thread. */
    for (i = 0; i < 5; i++){
        printf("Thread[%d]\ttdata_test = 0x%lX\n", id, tdata_test);
        tdata_test++;
        thd_sleep(50);
    }

    if(tdata_test != 0x5F) {
        fprintf(stderr, "TDATA data check failed!\n");
        ret = -1;
    }

    /* Ensure default-aligned .TBSS data is initialized properly. */
    for(int i = 0; i < 256; ++i)
        if(tls_uint16[i] != 0) {
            fprintf(stderr, "tls_uint16[%d] failed!\n", i);
            ret = -1;
            break;
        }

    /* Ensure manually over-aligned .TDATA data is initialized properly. */
    if(strcmp((const char *)tls_string, "abcdefghijklmnopqrstuvwxyz012345")) {
        fprintf(stderr, "tls_string check failed: %s\n", (const char *)tls_string);
        ret = -1;
    }

    /* Check if at least one byte has been offset improperly
       within either of these two oddly-sized structures which could
       be potentially misaligned.
       
       Thanks to the DevkitPro 3DS guys for creating this test case
       to flex their own TLS implementation. 
    */ 

    bool reproduced = false;

    printf("[");
    for (int i = 0; i < 3; i++) {
        if (tls_buff4.inner[i] != 2) 
            reproduced = true;
        
        printf("%d, ", tls_buff4.inner[i]);
    }
    printf("]\n");

    printf("[");
    for (int i = 0; i < 3; i++) {
        if (tls_buff16.inner[i] != 1) 
            reproduced = true;
        
        printf("%d, ", tls_buff16.inner[i]);
    }
    printf("]\n");

    if (reproduced) {
        fprintf(stderr, "Bug has been reproduced!\n");
        ret = -1;
    }
    else {
        printf("There has been no issue!\n");
    }

    printf("Finished Thread %d\n", id);

    /* Return the result back to the main function. */
    return (void *)ret;
}

int main(int argc, char **argv) {
    /* This is ridiculous, but lets do it anyway. */
    const int thread_count = 200; 
    kthread_t *threads[thread_count]; 
    int i, ret, result = 0; 

    printf("Starting Threads\n");

    /* Create a bunch of threads and put each through 
       the same series of tests on their own (hopefully)
       independent set of thread-local variables. */
    for (i = 0; i < thread_count; i++)
        threads[i] = thd_create(0, thd, (void *)i + 1);

    /* Put the main thread through the same tests as thread 0. */
    ret = (int)thd((void *)0);
    printf("Thread[0] returned: %d\n", ret);
    
    if(ret == -1)
        result = -1;

    /* Wait for each thread to return with its test result. */
    for(i = 0; i < thread_count; i++) {
        int res = thd_join(threads[i], (void**)&ret);

        if(res < 0) {
            fprintf(stderr, "Thread[%d] failed to join: %d\n", i + 1, res);
            result = -1;
        }
        else {
            printf("Thread[%d] returned: %d\n", i + 1, ret);
            
            if(ret == -1) 
                result = -1;
        }
    }
    
    printf("Threads Finished!\n");

    if(result == -1) {
        fprintf(stderr, "\n\n***** TLS TEST FAILED! *****\n\n");
        return EXIT_FAILURE;
    }
    else {
        printf("\n\n***** TLS TEST SUCCESS! *****\n\n");
        return EXIT_SUCCESS;
    }
}
