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
 * Nano Test Performance Functionality
 */

#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include <nano/nano.h>

#include "nanotest.h"
#include "system.h"

/*
 * Nanotest Performance Testing Framework.
 *
 * The performance testing framework needs to fulfill a few different
 * requirements:
 *
 *     (i)    Needs to support both SMP and uniprocessor systems,
 *            preferrably using the same testing code.
 *
 *     (ii)   Needs to support both reading cycle counters (via system calls)
 *            and using trace scripts (by configuring simulators).
 *
 * This framework attempts to fulfill these.
 *
 * Tests are created by using the macros 'PERF_TEST' or
 * 'MULTITHREADED_PERF_TEST', which specify a testing function. Testing
 * functions must then peform a start_test() and end_test() call, which will
 * start and stop the testing timers. The tests will be automatically rerun the
 * correct number of times, and torn down when testing is finished.
 *
 * The PERF_TEST and MULTITHREADED_PERF_TEST macros also accept a start and end
 * label. On platforms which perform their benchmarks by tracing the
 * processor's program counter, these labels can be used to specify the start
 * and end PC of the test. Typically, you will use the labels specified in the
 * start_test() and end_test() macros, but users may also specify a functions
 * or labels in other parts of the system, such as the kernel. This may be
 * useful, for instance, if the time between the a syscall and the system going
 * idle needs to be measured. Obviously, if this is the case, such tests will
 * not work on platforms that use cycle counters to perform the test.
 *
 * For instance, a basic test of IPC could be as follows:
 *
 *      PERF_TEST(IPC, 1000, "Test invalid IPC call", "ipc_start", "ipc_end",
 *           ipc_test_function);
 *
 *      static void ipc_test_function(int *tids)
 *      {
 *          start_test("ipc_start");
 *          ipc_call();
 *          end_test("ipc_end");
 *      }
 */


/*
 * Test thread prototype.
 *
 * Each testing thread must have this type signature. The
 * array 'tids' contains the tid of each thread created for
 * the test.
 */
typedef void (*test_thread_entry_t)(int *tids);

/*
 * Information about one (out of possibly more) thread that should be created
 * for a particular test.
 */
typedef struct test_thread
{
    test_thread_entry_t entry;
    int priority;
} test_thread_t;

/*
 * Description of a performance test.
 */
typedef struct performance_test
{
    /* Short test identifier. */
    char *name;

    /* Test number. */
    int number;

    /* What type of test is this? Possibilities include PERF_TRACED (requiring
     * PC logs to be parsed to get test results) or PERF_CYCLE_COUNTER
     * (allowing on-board cycle counters to be used.) */
    int type;

    /* Longer description of the test. */
    char *description;

    /* Number of execution units required for the test. If the
     * machine does not have this many execution units, the test
     * will be skipped. */
    int num_units;

    /* List of threads used in the test. The threads are started up in the
     * order listed. In general, this means that low priority threads should
     * generally be started up before higher priority threads so they have a
     * chance to complete any initialisation that they need to do. */
    test_thread_t *threads;

    /* Function to call at test startup. */
    void (*init_function)(void);

} performance_test_t;

/* Test types. */
#define PERF_CYCLE_COUNTER  0
#define PERF_TRACED         1

#define PERF_TEST(name, number, description, start_label,                     \
        end_label, type, function)                                            \
    /* Prototype of the testing function. */                                  \
    static void function(int *tids);                                          \
    /* Need a prototype for the test function */                              \
    static void __PERF_TEST_FUNC__ ## name ## number(void);                   \
                                                                              \
    /* Test threads array, containing only the single specified thread. */    \
    test_thread_t TEST_THREADS_ ## name ## number[] = {                       \
        {function, 1},                                                        \
        {NULL, 0},                                                            \
        };                                                                    \
                                                                              \
    /* Place into the ELF file information about the test. */                 \
    PERF_TEST_START_FUNCTION(name, number)                                    \
    PERF_TEST_INFO(name, number, description, start_label, end_label);        \
                                                                              \
    /* The peformance test data structure, under the name 'FOO_42'. */        \
    performance_test_t name ## _ ## number = {                                \
        #name, number, type, description, 1, TEST_THREADS_ ## name ## number, \
        __PERF_TEST_FUNC__ ## name ## number                                  \
    };

#define MULTITHREADED_PERF_TEST(name, number, description, start_label,       \
        end_label, type, num_units, functions)                                \
    /* Need a prototype for the test function */                              \
    static void __PERF_TEST_FUNC__ ## name ## number(void);                   \
    /* Place into the ELF file information about the test. */                 \
    PERF_TEST_START_FUNCTION(name, number)                                    \
    PERF_TEST_INFO(name, number, description, start_label, end_label);        \
                                                                              \
    /* The peformance test data structure, under the name 'FOO_42'. */        \
    performance_test_t name ## _ ## number = {                                \
        #name, number, type, description, num_units, functions,               \
        __PERF_TEST_FUNC__ ## name ## number                                  \
    };

/* Global variables to store the start and end cycles for the test. */
extern long perf_start_cycle;
extern long perf_end_cycle;

/* Tracing tests. */
#define start_tracing_test()
#define end_tracing_test()                                                    \
    do {                                                                      \
        perf_start_cycle = -1;                                                \
        perf_end_cycle = -1;                                                  \
        okn_syscall_ipc_send(1, 0, 0, 0, 0, 0, 0, 0, 0);                      \
    } while (0)

/* Convert a percentage into a parts-per-million value. */
#define PERCENT_TO_PPM(x)  ((x) * 10000)

/*
 * When scripts watch over our PC to work out what test we are trying to run,
 * they need to know what test we are currently running. We do this by doing
 * a call to a dummy function with a special name to let such tracing scripts
 * know where we are up to.
 */
#define PERF_TEST_START_FUNCTION(name, number)                                \
    void __PERF_TEST_FUNC__ ## name ## number (void) {                        \
        __asm__ __volatile__("" ::: "memory");                                \
    }

/* Performance test functions. */
int performance_test(performance_test_t *test, long *std_dev_ppm);
void setup_performance_tests(void);

/* Performance test categories. */
performance_test_t ** get_misc_perf_tests(void);
performance_test_t ** get_yield_perf_tests(void);
performance_test_t ** get_futex_perf_tests(void);
performance_test_t ** get_interrupt_perf_tests(void);
performance_test_t ** get_ipc_perf_tests(void);

/* Architecture specific performance stuff. */
#include <nanotest/arch/performance.h>

/* Should we display verbose test results to screen? */
#define VERBOSE_RESULTS     0

/* Number of runs to perform. */
#ifndef TEST_RUNS_STANDARD
#define TEST_RUNS_STANDARD     5
#endif
#ifndef WARMUP_RUNS_STANDARD
#define WARMUP_RUNS_STANDARD   2
#endif
#ifndef TEST_RUNS_TRACED
#define TEST_RUNS_TRACED       2
#endif

/* Should we convert all tests to traced tests? */
#ifndef FULL_TRACE_TESTRUN
#define FULL_TRACE_TESTRUN  0
#endif

/* Test start. */
#ifndef start_test
#define start_test(x)                                                         \
    do {                                                                      \
        relax();                                                              \
        /* Warm up perf_start/end_cycle and okn_syscall_get_cycles(). */      \
        perf_start_cycle = okn_syscall_get_cycles();                          \
        perf_end_cycle = okn_syscall_get_cycles();                            \
        /* Allow pipelines to empty. */                                       \
        __asm__ __volatile__("nop; nop; nop; nop\n");                         \
        __asm__ __volatile__(".balign 32\n");                                 \
        /* Start the test. */                                                 \
        perf_start_cycle = okn_syscall_get_cycles();                          \
        __asm__ __volatile__("nop; nop; nop; nop\n");                         \
        LABEL(x);                                                             \
    } while (0)
#endif

/* Test end. */
#ifndef end_test
#define end_test(x)                                                           \
    do {                                                                      \
        LABEL(x);                                                             \
        __asm__ __volatile__("nop; nop; nop; nop\n");                         \
        perf_end_cycle = okn_syscall_get_cycles();                            \
        __asm__ __volatile__("nop; nop; nop; nop\n");                         \
        okn_syscall_ipc_send(0, 0, 0, 0, 0, 0, 0, 0, 0);                      \
    } while (0)
#endif

#endif /* PERFORMANCE_H */

