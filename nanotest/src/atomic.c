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
 * Nano Thread Tests
 */

#include <stdio.h>
#include <stdlib.h>

#include <nano/nano.h>
#include <atomic_ops/atomic_ops.h>

#include <nanotest/nanotest.h>
#include <nanotest/system.h>
#include <nanotest/clock.h>

/*
 * ATOMIC1000 : Stress atomic operation support.
 *
 * We attempt to setup a situation causing atomic ops to fail, by having a low
 * priority thread performing constant atomic operations, and a high priority
 * thread preempting it and message with the same variable. This should
 * hopefully catch atomic operation problems.
 */

static okl4_atomic_word_t test1000_a;
static volatile int test1000_real_a;
static volatile int test1000_finished;

static void
test1000_child_thread(void *arg)
{
    int i = 0;

    /* Keep incrementing the counter until we are told to finish. */
    while (!test1000_finished) {
        okl4_atomic_inc(&test1000_a);
        i++;
    }
    test1000_real_a = i;
}

TEST(ATOMIC, 1000, "Stress atomic operation support")
{
    int pause_time;

    test1000_finished = 0;
    test1000_real_a = 0;
    okl4_atomic_set(&test1000_a, 0);

    /* Create a new thread. */
    int tid = create_thread(test1000_child_thread, 1, NULL);
    assert(tid >= 0);

    /* Work out how much time to wait between each preemption. */
    for (pause_time = 100;; pause_time = ((pause_time * 3) / 2) + 1) {
        int before = okl4_atomic_read(&test1000_a);
        pause(pause_time);
        int after = okl4_atomic_read(&test1000_a);
        if (after - before >= 10) {
            break;
        }
    }

    /* Pause for various amounts of time. */
    int my_increases = 0;
    for (int j = 0; j < 100; j++) {
        pause(pause_time);
        okl4_atomic_inc(&test1000_a);
        my_increases++;

        /* Wait a random number of cycles. */
        int r = rand() % 32;
        for (int i = 0; i < r; i++);
    }

    /* Wait for child to finish. */
    test1000_finished = 1;
    okn_syscall_thread_join(tid);

    /* Ensure that the counts are correct. */
    if (my_increases + test1000_real_a != okl4_atomic_read(&test1000_a)) {
        message("An error occurred: %d + %d != %ld\n",
                my_increases, test1000_real_a, (okl4_word_t)okl4_atomic_read(&test1000_a));
        return 1;
    }

    return 0;
}

/*
 * Thread tests
 */
struct nano_test **
get_atomic_tests(void)
{
    static struct nano_test *tests[] = {
        &ATOMIC_1000,
        NULL
        };
    return tests;
}

