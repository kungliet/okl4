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
 * Nano Futex Performance Tests
 */

#include <stdio.h>
#include <stdlib.h>

#include <nano/nano.h>
#include <atomic_ops/atomic_ops.h>

#include <nanotest/nanotest.h>
#include <nanotest/system.h>
#include <nanotest/performance.h>
#include <nanotest/clock.h>

/*
 * FUTEX1000 : Create pending signal
 */
PERF_TEST(PERF_FUTEX, 1000, "Create pending signal",
        "FUTEX1000_start", "FUTEX1000_end", PERF_CYCLE_COUNTER, futex1000);

static void
futex1000(int *tids)
{
    start_test("FUTEX1000_start");
    okn_syscall_futex_signal(1);
    end_test("FUTEX1000_end");
    okn_syscall_futex_wait(1);
}

/*
 * FUTEX1010 : Wait for pending signal
 */
PERF_TEST(PERF_FUTEX, 1010, "Wait for pending signal",
        "FUTEX1010_start", "FUTEX1010_end", PERF_CYCLE_COUNTER, futex1010)

static void
futex1010(int *tids)
{
    okn_syscall_futex_signal(2);
    start_test("FUTEX1010_start");
    okn_syscall_futex_wait(2);
    end_test("FUTEX1010_end");
}

/*
 * FUTEX1100 : Wait for signal, switch to idle.
 */
static volatile int futex1100_started;
static volatile int futex1100_ready;

static void
futex1100_helper(int *tids)
{
    /* Pause to allow main thread to do his thing. */
    pause(MEDIUM_PAUSE);

    /* Wake up the main thread. */
    okn_syscall_futex_signal(1100);

    /* Clean up. */
    end_tracing_test();
}

static void
futex1100_main(int *tids)
{
    /* Go to sleep. */
    LABEL("FUTEX1100_start");
    okn_syscall_futex_wait(1100);
}

static test_thread_t futex1100_functions[] = {
        {futex1100_main, 1},
        {futex1100_helper, 2},
        {NULL, 0}
};

MULTITHREADED_PERF_TEST(PERF_FUTEX, 1100, "Wait for signal, switch to idle",
        "FUTEX1100_start", "cpu_idle_sleep", PERF_TRACED, 1,
        futex1100_functions)

/*
 * FUTEX1110 : Wait for signal, switch to thread.
 */

static volatile int futex1110_ready;

static void
futex1110_main(int *tids)
{
    /* Wait for other thread to become ready. We will preempt him
     * in his spin loop when the timer fires. */
    futex1110_ready = 0;
    pause(SHORT_PAUSE);

    futex1110_ready = 1;
    start_test("FUTEX1110_start");
    okn_syscall_futex_wait(1110);
}

static void
futex1110_target(int *tids)
{
    /* Spin until we are allowed to continue. */
    while (!futex1110_ready);
    end_test("FUTEX1110_end");

    /* Wake up main thread again. */
    okn_syscall_futex_signal(1110);
}

static test_thread_t futex1110_functions[] = {
        {futex1110_main, 2},
        {futex1110_target, 1},
        {NULL, 0}
};

MULTITHREADED_PERF_TEST(PERF_FUTEX, 1110, "Wait for signal, switch to thread",
        "FUTEX1110_start", "FUTEX1110_end", PERF_CYCLE_COUNTER, 1,
        futex1110_functions);

/*
 * FUTEX1200 : Wake thread, preempting self
 */

static void
futex1200_main(int *tids)
{
    /* Go to sleep, waiting for second thread to wake us up. */
    okn_syscall_futex_wait(1200);
    end_test("FUTEX1200_end");
}

static void
futex1200_helper(int *tids)
{
    /* Wake up the main thread. */
    start_test("FUTEX1200_start");
    okn_syscall_futex_signal(1200);
}

static test_thread_t futex1200_functions[] = {
        {futex1200_main, 2},
        {futex1200_helper, 1},
        {NULL, 0}
};

MULTITHREADED_PERF_TEST(PERF_FUTEX, 1200, "Wake thread, preempting self",
        "FUTEX1200_start", "FUTEX1200_end", PERF_CYCLE_COUNTER, 1,
        futex1200_functions);

/*
 * FUTEX1210 : Wake thread, preempting spinner
 */

static volatile int futex1210_finished;

static void
futex1210_preemptee(int *tids)
{
    /* Spin until the test is finished. */
    futex1210_finished = 0;
    while (!futex1210_finished);
}

static void
futex1210_main(int *tids)
{
    /* Go to sleep, waiting for second thread to wake us up. */
    okn_syscall_futex_wait(1200);
    end_test("FUTEX1210_end");
    futex1210_finished = 1;
}

static void
futex1210_helper(int *tids)
{
    /* Wait for main thread to sleep, preemptee thread to sleep. */
    pause(MEDIUM_PAUSE);

    /* Wake up the main thread. */
    start_test("FUTEX1210_start");
    okn_syscall_futex_signal(1200);
}

static test_thread_t futex1210_functions[] = {
        {futex1210_preemptee, 1},
        {futex1210_main, 2},
        {futex1210_helper, 3},
        {NULL, 0}
};

MULTITHREADED_PERF_TEST(PERF_FUTEX, 1210, "Wake thread, preempting other",
        "FUTEX1210_start", "FUTEX1210_end", PERF_CYCLE_COUNTER, 3,
        futex1210_functions);

#if 0
/*
 * FUTEX1210 : Wake thread, preempting idle.
 */
PERF_TEST(PERF_FUTEX, 1220, "Wake thread, preempting idle",
        "FUTEX1200_start", "FUTEX1200_end")
{
    return futex1200_run_test(0, 0, 1);
}
#endif

#if 0
/*
 * FUTEX1210 : Wake thread and continue running.
 */
PERF_TEST(PERF_FUTEX, 1230, "Wake thread and continue running",
        "FUTEX1200_start", "FUTEX1200_end_syscall")
{
    return futex1200_run_test(NUM_EXECUTION_UNITS - 1,
            ROOT_TASK_PRIORITY + 1, 1);
}
#endif

/*
 * FUTEX2000 : Uncontended mutex lock
 */
PERF_TEST(PERF_FUTEX, 2000, "Uncontended mutex lock",
        "FUTEX2000_start", "FUTEX2000_end", PERF_CYCLE_COUNTER, futex2000);

static void
futex2000(int *tids)
{
    okn_futex_t lock = OKN_FUTEX_INIT;

    start_test("FUTEX2000_start");
    okn_futex_lock(&lock);
    end_test("FUTEX2000_end");
    okn_futex_unlock(&lock);
}

/*
 * FUTEX2000 : Uncontended mutex unlock
 */
PERF_TEST(PERF_FUTEX, 2010, "Uncontended mutex unlock",
        "FUTEX2010_start", "FUTEX2010_end", PERF_CYCLE_COUNTER, futex2010);

static void
futex2010(int *tids)
{
    okn_futex_t lock = OKN_FUTEX_INIT;

    okn_futex_lock(&lock);
    start_test("FUTEX2010_start");
    okn_futex_unlock(&lock);
    end_test("FUTEX2010_end");
}

/*
 * Futex Performance Tests
 */
performance_test_t **
get_futex_perf_tests(void)
{
    static performance_test_t *tests[] = {
        &PERF_FUTEX_1000,
        &PERF_FUTEX_1010,
        &PERF_FUTEX_1100,
        &PERF_FUTEX_1110,
        &PERF_FUTEX_1200,
        &PERF_FUTEX_1210,
        //&PERF_FUTEX_1220,
        //&PERF_FUTEX_1230,
        &PERF_FUTEX_2000,
        &PERF_FUTEX_2010,
        NULL
        };
    return tests;
}
