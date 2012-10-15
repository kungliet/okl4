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
 * Nano Performance Tests
 *
 * These tests don't actually _test_ performance, per-se, but rather just
 * execercise different paths in the kernel. The user is responsible for going
 * through the traces to get the actual results.
 */

#include <stdio.h>
#include <stdlib.h>

#include <nano/nano.h>
#include <atomic_ops/atomic_ops.h>

#include <nanotest/nanotest.h>
#include <nanotest/system.h>
#include <nanotest/clock.h>
#include <nanotest/performance.h>

/*
 * INT1000 : Wait for interrupt pending in hardware.
 *
 * When an interrupt fires without a handler being ready, it will
 * be blocked by the IAD register of the processor, and set the IPEND
 * register. This measures the time to receive an interrupt that already
 * has IPEND set.
 */
PERF_TEST(PERF_INT, 1000, "Wait for interrupt pending in hardware",
        "INT1000_start", "INT1000_end", PERF_CYCLE_COUNTER, int1000);

static void
int1000(int *tids)
{
    clock_register_interrupt();

    /* Start the clock, so that it triggers a pending interrupt. */
    clock_generate_interrupt(100);

    /* Wait for at least one tick. */
    for (int i = 0; i < 100*CYCLES_PER_TICK; i++);

    start_test("INT1000_start");
    okn_syscall_interrupt_wait();
    end_test("INT1000_end");

    while(!clock_process_interrupt())
        okn_syscall_interrupt_wait();

    clock_deregister_interrupt();
}

/*
 * INT1100 : Wait for interrupt, switch to idle.
 */

PERF_TEST(PERF_INT, 1100, "Wait for interrupt, switch to idle",
        "INT1100_start", "cpu_idle_sleep", PERF_TRACED, int1100);

static void
int1100(int *tids)
{
    /* Wake us up sometime in the future. */
    clock_register_interrupt();
    clock_generate_interrupt(MEDIUM_PAUSE);

    LABEL("INT1100_start");
    okn_syscall_interrupt_wait();
    end_tracing_test();

    while(!clock_process_interrupt())
        okn_syscall_interrupt_wait();

    clock_deregister_interrupt();
}

/*
 * INT1110 : Wait for interrupt, switch to thread.
 */

static volatile int int1110_ready;
static volatile int int1110_source_tid = -1;

static void
int1110_source(int *tids)
{
    int1110_ready = 0;
    int1110_source_tid = okn_syscall_thread_myself();

    /* Wait for main target thread to become ready. */
    okn_syscall_signal_wait(1);

    clock_register_interrupt();

    /* Do the wait. */
    int1110_ready = 1;
    start_test("INT1110_start");
    okn_syscall_interrupt_wait();

    while(!clock_process_interrupt())
        okn_syscall_interrupt_wait();

    clock_deregister_interrupt();
}

static void
int1110_target(int *tids)
{
    /* let the source thread start */
    while(int1110_source_tid == -1);
    okn_syscall_signal_send(int1110_source_tid, 1);

    /* Wait for the test to start. */
    while (!int1110_ready);

    /* Stop timing. */
    end_test("INT1110_end");

    /* Wake up main. */
    clock_generate_interrupt(100);
}

static test_thread_t int1110_functions[] = {
        {int1110_target, 1},
        {int1110_source, 2},
        {NULL, 0}
};

MULTITHREADED_PERF_TEST(PERF_INT, 1110, "Wait for interrupt, switch to thread",
        "INT1110_start", "INT1110_end", PERF_CYCLE_COUNTER, 1,
        int1110_functions);


/*
 * INT1200 : Interrupt trigger, preempting thread.
 */
static volatile int int1200_main_started;
static volatile int int1200_main_finished;

static void
int1200_source(int *tids)
{
    /* Wait for main thread to start. */
    while (!int1200_main_started);

    /*
     * Start the test.
     *
     * We have to trace this test, because we don't know when the interrupt
     * will fire.
     */
    LABEL("INT1200_start");
    clock_generate_interrupt(100);

    /* Wait for main thread to finish. */
    while (!int1200_main_finished);
}

static void
int1200_target(int *tids)
{
    int1200_main_finished = 0;
    int1200_main_started = 0;

    clock_register_interrupt();

    /* Go to sleep. */
    int1200_main_started = 1;
    okn_syscall_interrupt_wait();
    LABEL("INT1200_end");
    end_tracing_test();

    while(!clock_process_interrupt())
        okn_syscall_interrupt_wait();

    /* Clean up. */
    int1200_main_finished = 1;
    clock_deregister_interrupt();
}

static test_thread_t int1200_functions[] = {
        {int1200_target, 2},
        {int1200_source, 1},
        {NULL, 0}
};

MULTITHREADED_PERF_TEST(PERF_INT, 1200, "Interrupt trigger, preempting thread",
        "INT1200_start", "INT1200_end", PERF_TRACED, 1, int1200_functions);

/*
 * INT1200 : Interrupt trigger, preempting idle.
 */

PERF_TEST(PERF_INT, 1210, "Interrupt trigger, preempting idle",
        "dummy_idle_thread", "INT1210_end", PERF_TRACED, int1210);

static void
int1210(int *tids)
{
    clock_register_interrupt();
    /* Start clock in the future. */
    clock_generate_interrupt(MEDIUM_PAUSE);

    /* Go to sleep. */
    okn_syscall_interrupt_wait();
    LABEL("INT1210_end");
    end_tracing_test();

    while(!clock_process_interrupt())
        okn_syscall_interrupt_wait();
    
    clock_deregister_interrupt();
}

/*
 * INT1300 : Interrupt trigger, preempting high-priority thread.
 *
 * In this case, an interrupt triggers preempting a thread that is not the
 * lowest priority thread running. This thread should, in turn, preempt the
 * real low priority thread in the system.
 *
 * This test is quite hard to run. We have a high priority thread that rapidly
 * increases a counter. We have a few medium priority threads that read this
 * counter, seeing if they detect any jitter in it. Finally, we have a low
 * priority thread that should be preempted.
 *
 * The high priority thread sets off the interrupt. If it goes to one of the
 * medium priority threads, they should detect the jitter, and we get a
 * test result. If it happens to go to the low priority thread, no one
 * will detect any jitter, and the test needs to be run again.
 */

/* Amount of jitter to assume a preemption. */
#define INT1300_JITTER_THRESHOLD  10

/* Range of clock ticks to try. */
#define INT1300_MIN_WAIT_TIME     10
#define INT1300_MAX_WAIT_TIME     30

/* Number of times to loop before assuming timer hit. */
#define INT1300_LOOP_ITERATIONS   2000

static volatile int int1300_jitter;
static volatile int int1300_jitter_detected;
static volatile int int1300_medium_preempted;
static volatile int int1300_medium_ready;
static volatile int int1300_low_preempted;
static volatile int int1300_low_ready;
static volatile int int1300_finished;
static volatile int int1300_test_success;
static volatile int int1300_handler_done;
static okn_barrier_t int1300_barrier
        = OKN_BARRIER_INIT(NUM_EXECUTION_UNITS + 1);

static void
int1300_handler(int *tids)
{
    clock_register_interrupt();

    /* We keep trying to do the test until everyone is happy. */
    do {
        okn_barrier(&int1300_barrier);

        /* Receive a single interrupt. */
        okn_syscall_interrupt_wait();

        int1300_handler_done = 1;

        /* Make sure the jitter testing functions have started. */
        assert(int1300_jitter > 0);

        /* Wait for everyone else to finish. */
        while (!int1300_finished);

        /* Finish up. */
        while(!clock_process_interrupt())
            okn_syscall_interrupt_wait();

        okn_barrier(&int1300_barrier);
        okn_barrier(&int1300_barrier);
    } while (!int1300_test_success);

    clock_deregister_interrupt();
}

static int
int1300_detect_jitter(void)
{
    int previous = int1300_jitter;
    int maximum_jitter = -1;
    int last_loop = 0;

    do {
        /* Should this be our last loop? */
        last_loop = int1300_finished != 0;

        /* Read current jutter. */
        int current = int1300_jitter;

        /* If it exceeds the maximum, use it. */
        if (current - previous > maximum_jitter) {
            maximum_jitter = current - previous;
        }

        previous = current;
    } while (!last_loop);

    return maximum_jitter;
}

static void
int1300_jitter_detector(int *tids)
{

    do {
        int1300_medium_ready = 1;
        okn_barrier(&int1300_barrier);

        int jitter = int1300_detect_jitter();
        if (jitter > INT1300_JITTER_THRESHOLD) {
            int1300_jitter_detected = 1;
        }

        okn_barrier(&int1300_barrier);
        okn_barrier(&int1300_barrier);
    } while (!int1300_test_success);
}

static void
int1300_low_thread(int *tids)
{
    do {
        int1300_low_ready = 1;
        okn_barrier(&int1300_barrier);

        /* Wait to be preempted and the test to finish. */
        int1300_detect_jitter();

        okn_barrier(&int1300_barrier);
        okn_barrier(&int1300_barrier);
    } while (!int1300_test_success);
}

static void
int1300_master(int *tids)
{
    int1300_test_success = 0;

    for (int n = 0; n < 16; n++) {
        /* Setup shared memory. */
        int1300_jitter = 0;
        int1300_medium_preempted = 0;
        int1300_low_preempted = 0;
        int1300_finished = 0;
        int1300_jitter_detected = 0;
        int1300_handler_done = 0;
        okn_barrier(&int1300_barrier);

        /* Start increasing the timer. */
        relax();
        clock_generate_interrupt(MEDIUM_PAUSE + n);
        LABEL("INT1300_start");
        for (int i = 0; !int1300_handler_done; i++)
            int1300_jitter = i;
        LABEL("INT1300_end");

        /* Finish up threads. */
        int1300_finished = 1;

        /* Wait for everyone to finish up. */
        okn_barrier(&int1300_barrier);
        if (int1300_jitter_detected)
            break;

        /* No jitter was detected. Restart the test. */
        okn_barrier(&int1300_barrier);
    }

    /* Clean up. */
    end_tracing_test();
    int1300_test_success = 1;
    okn_barrier(&int1300_barrier);
}

static test_thread_t int1300_functions[] = {
        {int1300_master, 4},
        {int1300_handler, 3},
        {int1300_jitter_detector, 2},
        {int1300_jitter_detector, 2},
        {int1300_jitter_detector, 2},
        {int1300_jitter_detector, 2},
        {int1300_low_thread, 1},
        {NULL, 0}
};

MULTITHREADED_PERF_TEST(PERF_INT, 1300, "Interrupt trigger, preempting thread",
        "INT1300_start", "INT1300_end", PERF_TRACED, 6, int1300_functions);

/*
 * Performance Tests
 */
performance_test_t **
get_interrupt_perf_tests(void)
{
    static performance_test_t *tests[] = {
        &PERF_INT_1000,
        &PERF_INT_1100,
        &PERF_INT_1110,
        &PERF_INT_1200,
        &PERF_INT_1210,
        &PERF_INT_1300,
        NULL
        };
    return tests;
}

