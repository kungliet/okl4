/*
 * Copyright (c) 2008 Open Kernel Labs, Inc. (Copyright Holder).
 * All rights reserved.
 *
 * 1. Redistribution and use of OKL4 (Software) in source and binary
 * forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 *     (a) Redistributions of source code must retain this clause 1
 *         (including paragraphs (a), (b) and (c)), clause 2 and clause 3
 *         (Licence Terms) and the above copyright notice.
 *
 *     (b) Redistributions in binary form must reproduce the above
 *         copyright notice and the Licence Terms in the documentation and/or
 *         other materials provided with the distribution.
 *
 *     (c) Redistributions in any form must be accompanied by information on
 *         how to obtain complete source code for:
 *        (i) the Software; and
 *        (ii) all accompanying software that uses (or is intended to
 *        use) the Software whether directly or indirectly.  Such source
 *        code must:
 *        (iii) either be included in the distribution or be available
 *        for no more than the cost of distribution plus a nominal fee;
 *        and
 *        (iv) be licensed by each relevant holder of copyright under
 *        either the Licence Terms (with an appropriate copyright notice)
 *        or the terms of a licence which is approved by the Open Source
 *        Initative.  For an executable file, "complete source code"
 *        means the source code for all modules it contains and includes
 *        associated build and other files reasonably required to produce
 *        the executable.
 *
 * 2. THIS SOFTWARE IS PROVIDED ``AS IS'' AND, TO THE EXTENT PERMITTED BY
 * LAW, ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, OR NON-INFRINGEMENT, ARE DISCLAIMED.  WHERE ANY WARRANTY IS
 * IMPLIED AND IS PREVENTED BY LAW FROM BEING DISCLAIMED THEN TO THE
 * EXTENT PERMISSIBLE BY LAW: (A) THE WARRANTY IS READ DOWN IN FAVOUR OF
 * THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
 * PARTICIPANT) AND (B) ANY LIMITATIONS PERMITTED BY LAW (INCLUDING AS TO
 * THE EXTENT OF THE WARRANTY AND THE REMEDIES AVAILABLE IN THE EVENT OF
 * BREACH) ARE DEEMED PART OF THIS LICENCE IN A FORM MOST FAVOURABLE TO
 * THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
 * PARTICIPANT). IN THE LICENCE TERMS, "PARTICIPANT" INCLUDES EVERY
 * PERSON WHO HAS CONTRIBUTED TO THE SOFTWARE OR WHO HAS BEEN INVOLVED IN
 * THE DISTRIBUTION OR DISSEMINATION OF THE SOFTWARE.
 *
 * 3. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ANY OTHER PARTICIPANT BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Framework to test Nano system calls and functionality.
 */

#include <stdio.h>
#include <stdlib.h>

#include <nano/nano.h>

#include <nanotest/nanotest.h>
#include <nanotest/performance.h>
#include <nanotest/system.h>
#include <nanotest/clock.h>

/* Entry point. */
int main(void);

/* Get the overhead associated with reading the cycle counter. */
static long long
get_cycle_counter_overhead(void)
{
    static int a = 0;
    if (a != 0)
        return a;

    /* Prime the caches. */
    okn_syscall_get_cycles();
    okn_syscall_get_cycles();

    /* Measure. */
    int before = okn_syscall_get_cycles();
    int after = okn_syscall_get_cycles();

    /* Return the difference. */
    return a = (after - before);
}


/*
 * Run a series of tests.
 *
 * Return the number of tests that failed.
 */
static void
run_tests(struct nano_test **tests, int *tests_run, int *failures)
{
    /* Run the tests. */
    for (int i = 0; tests[i] != NULL; i++) {
        /* Run the test. */
        printf("%12s%04d: (%s)\n", tests[i]->category,
                tests[i]->number, tests[i]->name);
        (*tests_run)++;

        /* Is the test timed? */
        int test_failed;
        if (tests[i]->type == TEST_TYPE_TIMED) {
            long long start_time = okn_syscall_get_cycles();
            test_failed = tests[i]->function();
            long long end_time = okn_syscall_get_cycles();
            message("Time taken: %lld kcycle(s)\n",
                    (end_time - start_time - get_cycle_counter_overhead())
                            / (NUM_EXECUTION_UNITS * 1000));
        } else {
            test_failed = tests[i]->function();
        }

        /* Did the test succeed? */
        if (test_failed) {
            printf("*** FAILED ***\n");
            (*failures)++;
        }
    }
    printf("\n");
}

/*
 * Run some performance tests.
 */
static void
run_perf_tests(struct performance_test **tests, int *tests_run, int *failures)
{
    /* Run the tests. */
    for (int i = 0; tests[i] != NULL; i++) {
        /* If the test requires more execution units than we have, skip
         * it. */
        if (tests[i]->num_units > NUM_EXECUTION_UNITS)
            continue;

        /* Run the test. */
        printf("%12s%04d: (%s)\n", tests[i]->name,
                tests[i]->number, tests[i]->description);
        (*tests_run)++;

        long error_ppm;
        int result = performance_test(tests[i], &error_ppm);
        if (result >= 0) {
            message("%d cycles\n", result);
            if (error_ppm > PERCENT_TO_PPM(1))
                message("Warning: noisy test, error exceeds %ld.%02ld%%.\n",
                        error_ppm / 10000, (error_ppm / 100) % 100);
        } else {
            message("Benchmark requires tracing.\n");
        }
    }
    printf("\n");
}

static void
setup_serial(void)
{
    printf("Nanotest setup: serial appears to work.\n");
}

/*
 * Entry point.
 */
int
main(void)
{
    int tests_run = 0, failures = 0;

    /* Init. */
    arch_init();
    setup_serial();
    clock_init();
    setup_performance_tests();
    flush_timer_interrupt();

    puts("\n"
         "NanoTest\n"
         "-------\n");

    /* Performance */
    run_perf_tests(get_misc_perf_tests(), &tests_run, &failures);
    run_perf_tests(get_yield_perf_tests(), &tests_run, &failures);
    run_perf_tests(get_futex_perf_tests(), &tests_run, &failures);
    run_perf_tests(get_ipc_perf_tests(), &tests_run, &failures);
    run_perf_tests(get_interrupt_perf_tests(), &tests_run, &failures);

    /* Functionality */
    run_tests(get_ipc_tests(), &tests_run, &failures);
    run_tests(get_interrupt_tests(), &tests_run, &failures);
    run_tests(get_futex_tests(), &tests_run, &failures);
    run_tests(get_thread_tests(), &tests_run, &failures);
    run_tests(get_signal_tests(), &tests_run, &failures);
    run_tests(get_atomic_tests(), &tests_run, &failures);

    printf("\nNanoTest Complete: %d tests run, %d failures.\n",
            tests_run, failures);

    system_reset();
}

