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


#include <stdio.h>
#include <nano/nano.h>

#include <nanotest/performance.h>
#include <nanotest/system.h>
#include <nanotest/clock.h>

/* Global performance counter value storage. */
long perf_start_cycle;
long perf_end_cycle;

/* Initialisation globals. */
static int overhead = 0;
static int performance_tests_setup = 0;

PERF_TEST(NULL_TEST, 0, "IPC send to waiting thread. (Closed wait.)",
        "null_test_before", "null_test_after", PERF_CYCLE_COUNTER, null_test);

static void
null_test(int *tids)
{
    start_test("null_test_before");
    end_test("null_test_after");
}

/* Entry point for threads created by "test_runner". */
void thread_bootstrap(void *arg);
void
thread_bootstrap(void *arg)
{
    test_thread_entry_t entry_point;
    volatile int *started;
    volatile int *finished;
    int *tids;
    int repeats;
    okn_barrier_t *barrier;

    /* Wait for parent to start us with our thread entry point,
     * pointers to our starting/finishing variables, and the
     * TIDs of the other threads. */
    okn_syscall_ipc_recv(
            (unsigned long *)(void *)&entry_point,
            (unsigned long *)(void *)&started,
            (unsigned long *)(void *)&finished,
            (unsigned long *)(void *)&tids,
            (unsigned long *)(void *)&repeats,
            (unsigned long *)(void *)&barrier,
            0, 0, (int)arg, 0);
    okn_barrier(barrier);

    /* Call the thread. */
    for (int i = 0; i < repeats; i++) {
        /* Wait until test is started. */
        while (!*started);

        /* Carry out this test. */
        entry_point(tids);

        /* Wait until this test is finished. */
        while (!*finished);

        /* Wait for everyone else. */
        okn_barrier(barrier);
        okn_barrier(barrier);
    }

}

void
run_test(performance_test_t *test, int repeats,
        long long *results, int test_type);
void
run_test(performance_test_t *test, int repeats,
        long long *results, int test_type)
{
    /* Calculate the number of threads. */
    int num_threads = 0;
    while (test->threads[num_threads].entry != NULL)
        num_threads++;
    assert(num_threads > 0);

    /* Create each thread, but don't let them run just yet. */
    int tids[num_threads];
    for (int i = 0; i < num_threads; i++) {
        int tid = create_thread(thread_bootstrap,
                test->threads[i].priority,
                (void *)okn_syscall_thread_myself());
        assert(tid >= 0);
        tids[i] = tid;
    }

    /* Setup barrier for between test runs. */
    okn_barrier_t barrier;
    okn_barrier_init(&barrier, num_threads + 1);

    /* Now that we have the full list of TIDs, we can start the threads
     * one by one. */
    volatile int started = 0;
    volatile int finished = 0;
    for (int i = 0; i < num_threads; i++) {
        okn_syscall_ipc_send((int)test->threads[i].entry,
                (int)&started, (int)&finished, (int)tids,
                repeats, (int)&barrier, 0, tids[i], 0);
    }
    okn_barrier(&barrier);

    /* Start up the appropriate number of spinners. */
    assert(test->num_units > 0);
    if (NUM_EXECUTION_UNITS - test->num_units > 0) {
        create_spinners(NUM_EXECUTION_UNITS - test->num_units, 31);
    }

#if VERBOSE_RESULTS
    if (test_type == PERF_CYCLE_COUNTER && !FULL_TRACE_TESTRUN) {
        message("Assuming %d cycle test overhead\n", overhead);
    }
#endif

    /* Repeat the test multiple times. */
    for (int r = repeats; r > 0; r--) {

        /* Call test init function. */
        test->init_function();
        relax();

        /* Allow everyone to start. */
        started = 1;

        /* Wait for someone to report back to use that the test is finished. */
        okn_syscall_ipc_recv(0, 0, 0, 0, 0, 0, 0, 0, OKN_IPC_OPEN_WAIT, 0);
        finished = 1;

        /* Record the result. */
        results[repeats - r] = (perf_end_cycle - perf_start_cycle) / NUM_EXECUTION_UNITS;
#if VERBOSE_RESULTS
        if (test_type == PERF_TRACED || FULL_TRACE_TESTRUN) {
            message("traced test iteration complete\n");
        } else {
            message("result %d : %d cycles\n", (int)(repeats - r), (int)results[repeats - r]);
        }
#endif

        /* Wait for everyone to finish up. */
        okn_barrier(&barrier);
        started = 0;
        finished = 0;
        okn_barrier(&barrier);
    }

    /* Delete spinners. */
    delete_spinners();

    /* Wait for all threads to clean themselves up. */
    for (int i = 0; i < num_threads; i++) {
        okn_syscall_thread_join(tids[i]);
    }
}

/*
 * Calculate the square root of the input integer using only
 * integer arithmatic.
 */
static long
isqrt(long long n)
{
    if (n == 0)
        return 0;

    long long xn = 1;
    long long xn1 = (xn + n / xn) / 2;
    while (abs(xn1 - xn) > 1) {
        xn = xn1;
        xn1 = (xn + n / xn) / 2;
    }
    while (xn1 * xn1 > n) {
        xn1 -= 1;
    }

    return (long)xn1;
}

static void
calculate_statistics(long long *data, int samples, long *avg, long *std_dev_ppm)
{
    long long sum = 0;
    long long sum_sqrs = 0;

    /* Calculate average, average square. */
    for (int i = 0; i < samples; i++) {
        sum += data[i];
        sum_sqrs += data[i] * data[i];
    }

    /* Determine final averages and error margins. */
    long long x = sum / samples;                      /* E[X]   */
    long long y = sum_sqrs / samples;                 /* E[X^2] */
    long long z = (sum * sum) / (samples * samples);  /* E[X]^2 */
    long long variance = y - z;                       /* E[X^2] - E[X]^2 */

    /*
     * std_dev_ppm = (sqrt(variance) / average) * 1000000
     *
     * We mess with the formula a little to minimise effects of integer
     * truncation and to reduce chances of overflow.
     */
    if (x > 0) {
        *std_dev_ppm = (int)(10000LL * isqrt(10000LL * variance) / x);
    } else {
        *std_dev_ppm = variance > 0 ? 1000000 : 0;
    }
    *avg = (int)x;
}

static int
get_raw_result(performance_test_t *test, long *std_dev_ppm)
{
    /* If this test requires tracing of PC logs, don't worry about
     * calculating complex results. */
    if (test->type == PERF_TRACED || FULL_TRACE_TESTRUN) {
        long long results[TEST_RUNS_TRACED];
        run_test(test, TEST_RUNS_TRACED, results, PERF_TRACED);
        return -1;
    }

    /* Otherwise, run the test to do cycle counter measurements. */
    long long results[TEST_RUNS_STANDARD];

    /* Kill cache, so we start running the tests in a known state. */
    kill_cache();

    /* Run the tests. */
    run_test(test, TEST_RUNS_STANDARD, results, PERF_CYCLE_COUNTER);

    /*
     * We discard the first few results, to take into account things such as:
     *
     *     (i)    TLB misses;
     *     (ii)   mini-TLB misses (used by certain proprietary processors);
     *     (iii)  cache misses;
     *     (iv)   warming up branch predictors that buffer branch results
     *            (for instance, ARM X-Scale);
     *     (v)    bad luck (for instance, IA-32).
     *
     * You would not be blamed for thinking that 1 or 2 passes would be
     * sufficient to cover most of these, but empirical results show that the
     * results are often still not stable until the fourth or fifth test run.
     * It is not always clear why this is the case; though random cache/TLB
     * replacement algorithms may require quite a few runs before they
     * stabilise.
     *
     * Once we have discarded the first few results, we calculate the average
     * and the standard deviation (taken as parts-per-million of the returned
     * result). We do all this in integer arithmatic because certain platforms
     * do not have library/compiler support for floating point.
     */
    long avg;
    calculate_statistics(results + WARMUP_RUNS_STANDARD,
            TEST_RUNS_STANDARD - WARMUP_RUNS_STANDARD, &avg, std_dev_ppm);
    return avg;
}

void
setup_performance_tests(void)
{
    long std_dev;
    long r = get_raw_result(&NULL_TEST_0, &std_dev);

    /* Do we have sane counter results? */
    if (r == 0) {
        printf("*** Warning: No functioning cycle counter available.\n");
    } else if (std_dev >= PERCENT_TO_PPM(1)) {
        printf("*** Warning: Large amounts of noise measured in sample test.\n");
        printf("             Error margin %ld ppm.\n", std_dev);
    }
    printf("Performance framework: assuming %ld cycle overhead "
            "to read cycle counters.\n", r);
    overhead = r;
    performance_tests_setup = 1;
}

int
performance_test(performance_test_t *test, long *std_dev_ppm)
{
    /* Ensure that we have setup the tests. */
    assert(performance_tests_setup);

    /* Run the test. */
    long result = get_raw_result(test, std_dev_ppm);
    if (result < 0) {
        return -1;
    }

    /* Return the cycle-counted result. */
#if VERBOSE_RESULTS
    message("%d cycles total, error %d ppm.\n",
            (int)(result - overhead), (int)*std_dev_ppm);
#endif
    return result - overhead;
}

