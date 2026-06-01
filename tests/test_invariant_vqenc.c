#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <signal.h>
#include <setjmp.h>

/* Test that vqenc handles long filenames without buffer overflow.
 * Invariant: Processing any filename must not corrupt memory or crash. */

static jmp_buf jump_buffer;

static void segfault_handler(int sig) {
    longjmp(jump_buffer, 1);
}

/* We invoke the vqenc binary as a subprocess to test the real code path
 * without crashing the test process itself. */
START_TEST(test_filename_buffer_overflow)
{
    /* Invariant: vqenc must not crash/segfault regardless of input filename length */
    const char *payloads[] = {
        /* Exact exploit: filename long enough to overflow a fixed buffer + extension */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA.vqf",
        /* Boundary: exactly 256 chars before extension */
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB.vqf",
        /* Valid: normal short filename */
        "test.vqf",
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        char cmd[4096];
        /* Run vqenc with the payload as argument; capture exit status only */
        snprintf(cmd, sizeof(cmd),
                 "timeout 5 ./utils/vqenc/vqenc %s > /dev/null 2>&1; "
                 "STATUS=$?; [ $STATUS -ne 139 ] && [ $STATUS -ne 134 ]",
                 payloads[i]);
        int ret = system(cmd);
        /* Assert: process must NOT have died with SIGSEGV (139) or SIGABRT (134) */
        ck_assert_msg(ret == 0,
                      "vqenc crashed (segfault/abort) on payload index %d: %s",
                      i, payloads[i]);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");
    tcase_set_timeout(tc_core, 30);
    tcase_add_test(tc_core, test_filename_buffer_overflow);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}