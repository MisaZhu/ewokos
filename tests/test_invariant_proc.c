#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

// Include the actual header if available, otherwise declare needed functions
#include "kernel/kernel/src/proc.h"

START_TEST(test_buffer_reads_never_exceed_declared_length)
{
    // Invariant: Buffer reads never exceed the declared length
    const char *payloads[] = {
        "normal",                    // Valid input
        "A" * 255,                   // Boundary case (assuming 256 byte buffer)
        "EXPLOIT" * 1000,            // Oversized input (7000 bytes)
        "\x00\x01\x02\x03" * 64,     // Binary data pattern
        "A" * 1024                   // Another oversized case
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        // Create a test file with the payload
        FILE *test_file = fopen("test_payload.txt", "w");
        ck_assert_ptr_nonnull(test_file);
        fwrite(payloads[i], strlen(payloads[i]), 1, test_file);
        fclose(test_file);

        // Fork and execute the actual code path that reads the buffer
        pid_t pid = fork();
        if (pid == 0) {
            // Child process: execute the actual vulnerable function
            // This assumes there's a function that processes input from a file
            extern void process_file(const char *filename); // Declare if not in header
            process_file("test_payload.txt");
            _exit(0); // Should not reach here if buffer overflow crashes
        } else {
            // Parent process: wait with timeout
            int status;
            waitpid(pid, &status, 0);
            
            // Check if child terminated normally (not by signal)
            ck_assert_msg(WIFEXITED(status), 
                         "Buffer overflow detected with payload %d (size: %zu)", 
                         i, strlen(payloads[i]));
        }

        // Clean up
        unlink("test_payload.txt");
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_reads_never_exceed_declared_length);
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