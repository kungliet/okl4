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
 * Nano Yield Performance Tests
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
 * YIELD1000 : Yield to self
 */
PERF_TEST(PERF_YIELD, 1000, "Yield to self",
        "YIELD1000_start", "YIELD1000_end", PERF_CYCLE_COUNTER,
        yield1000_thread);

static void
yield1000_thread(int *tids)
{
    start_test("YIELD1000_start");
    okn_syscall_yield();
    end_test("YIELD1000_end");
}


/*
 * YIELD1010 : Yield to next thread.
 */
static okn_barrier_t yield1100_barrier = OKN_BARRIER_INIT(2);
static volatile int yield1100_test_started;
static volatile int yield1100_second_thread_started;

static void
yield1100_first(int *tids)
{
    /* Wait for other thread to start. */
    yield1100_test_started = 0;
    okn_barrier(&yield1100_barrier);

    /* Wait for second thread to get ready out of the barrier. */
    while (!yield1100_second_thread_started)
        okn_syscall_yield();

    yield1100_test_started = 1;
    start_test("YIELD1100_start");
    okn_syscall_yield();
}

static void
yield1100_second(int *tids)
{
    /* Indicate that we haven't started yet. */
    yield1100_second_thread_started = 0;

    /* Wait for other thread to start. */
    okn_barrier(&yield1100_barrier);

    /*
     * Yield to other thread until we find out we are timing.
     *
     * FIXME : Work out how to remove the overhead of the
     * memory load / compare to exit the while() loop.
     */
    yield1100_second_thread_started = 1;
    while (!yield1100_test_started) {
        okn_syscall_yield();
    }

    /* Control returned back to us.*/
    end_test("YIELD1100_end");
}

test_thread_t yield1100_threads[] = {
    {yield1100_first, 1},
    {yield1100_second, 1},
    {NULL, 0},
    };

MULTITHREADED_PERF_TEST(PERF_YIELD, 1100, "Yield to another thread",
        "YIELD1100_start", "YIELD1100_end", PERF_CYCLE_COUNTER,
        1, yield1100_threads);

/*
 * Performance Tests
 */
struct performance_test **
get_yield_perf_tests(void)
{
    static struct performance_test *tests[] = {
        &PERF_YIELD_1000,
        &PERF_YIELD_1100,
        NULL
        };
    return tests;
}

