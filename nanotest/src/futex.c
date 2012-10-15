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
 * Nano Futex Tests
 */

#include <stdio.h>
#include <stdlib.h>

#include <nano/nano.h>
#include <atomic_ops/atomic_ops.h>

#include <nanotest/nanotest.h>
#include <nanotest/system.h>

/*
 * FUTEX1000 : Futex Signal/Wait.
 *
 * Ensure that we can signal a futex and then wait on it.
 */
TEST(FUTEX, 1000, "Futex Signal/Wait")
{
    int error;

    /* Signal threads. */
    error  = okn_syscall_futex_signal(0xdeadbeef);
    error |= okn_syscall_futex_signal(0xdeadbeef);
    error |= okn_syscall_futex_signal(0xfeedcafe);
    error |= okn_syscall_futex_signal(0xfeedcafe);
    error |= okn_syscall_futex_wait(0xdeadbeef);
    error |= okn_syscall_futex_wait(0xfeedcafe);
    error |= okn_syscall_futex_wait(0xfeedcafe);
    error |= okn_syscall_futex_wait(0xdeadbeef);
    assert(!error);

    return 0;
}

/*
 * FUTEX1010 : Null futex call.
 *
 * Ensure calling futex_wait(0) returns an error.
 */
TEST(FUTEX, 1010, "Null Futex Call")
{
    int error;
    error = okn_syscall_futex_wait(0);
    assert(error);

    return 0;
}

/*
 * FUTEX1011 : Futex Stored Signal.
 *
 * Overload the maximum number of stored signals
 */
TEST(FUTEX, 1011, "Futex Signal/Wait")
{
    int error = 0;
    int signals = 0;
    int i;

    /* Signal threads. */
    while(!error) {
        error  = okn_syscall_futex_signal(0xdeadbeef);
        signals++;
    }
    
    signals --;

    /* Consume signals */
    for(i = 0; i < signals; i++) {
        error = okn_syscall_futex_wait(0xdeadbeef);
        assert(!error);
    }

    return 0;
}

/*
 * FUTEX1100 : Futex Wait/Signal.
 *
 * Wait for a thread to sleep on a futex, and then signal
 * it.
 */

volatile int futex1100_flag;

static void
futex1100_child(void *args)
{
    int error;

    /* Wait on the futex. */
    error = okn_syscall_futex_wait(0x12345678);
    assert(!error);

    /* Ensure parent has signalled us, and the syscall didn't
     * slip straight through. */
    assert(futex1100_flag == 1);

    /* Inform our parent we have woken and quit. */
    futex1100_flag = 2;
}


TEST(FUTEX, 1100, "Futex Wait/Signal")
{
    int error;
    futex1100_flag = 0;

    /* Only run this test on a single execution unit. */
    create_spinners(NUM_EXECUTION_UNITS - 1, ROOT_TASK_PRIORITY + 2);

    /* Create a child thread, and let them preempt us. */
    int tid = create_thread(futex1100_child, ROOT_TASK_PRIORITY + 1, NULL);
    assert(tid >= 0);

    /* Wake our child, preempting us. */
    futex1100_flag = 1;
    error = okn_syscall_futex_signal(0x12345678);
    assert(!error);

    /* Ensure our child ran. */
    assert(futex1100_flag == 2);

    /* wait for child to exit */
    okn_syscall_thread_join(tid);

    /* Clean up spinners. */
    delete_spinners();

    okn_syscall_thread_join(tid);

    return 0;
}

/*
 * FUTEX1110 : Many threads waiting.
 *
 * Multiple threads created, all waiting on random futexes. The idea here is
 * that we try to get a in-kernel hash collision. The birthday-paradox implies
 * that when we get O(sqrt(MAX_KERNEL_THREADS)) sleeping threads, we have a 50%
 * chance of a collision.
 */

#define FUTEX1110_ROUNDS 5
#define MAX_KERNEL_THREADS 31

static unsigned int futex1110_wait_tag[MAX_KERNEL_THREADS];
static okn_barrier_t futex1110_barrier;
static volatile int futex1110_should_be_sleeping;

static void
futex1110_child(void *arg)
{
    int me = (int)arg;
    int error;

    (void)error;

    for (int i = 0; i < FUTEX1110_ROUNDS; i++) {
        okn_barrier(&futex1110_barrier);
        error = okn_syscall_futex_wait(futex1110_wait_tag[me]);
        assert(!error);
        assert(!futex1110_should_be_sleeping);
        okn_barrier(&futex1110_barrier);
    }
}

TIMED_TEST(FUTEX, 1110, "Many threads waiting")
{
    int tid[MAX_KERNEL_THREADS];
    int error;

    (void) error;

    /* Create the barrier. */
    okn_barrier_init(&futex1110_barrier, MAX_KERNEL_THREADS + 1);

    /* Create a bunch of threads. */
    for (int i = 0; i < MAX_KERNEL_THREADS; i++) {
        for (int j = 0; j < 1000; j++);
        tid[i] = create_thread(futex1110_child, 1, (void *)i);
        assert(tid[i] >= 0);
    }

    /* Run a few rounds. */
    for (int i = 0; i < FUTEX1110_ROUNDS; i++) {

        /* Generate random numbers for threads to wait on. */
        int r = 363714;
        for (int j = 0; j < MAX_KERNEL_THREADS; j++) {
            futex1110_wait_tag[j] = r;
            r *= 2003;
            r %= 998497;
            r += 1;
        }

        /* Allow threads to start. */
        okn_barrier(&futex1110_barrier);
        futex1110_should_be_sleeping = 1;

        /* Wake up threads. */
        futex1110_should_be_sleeping = 0;
        for (int j = MAX_KERNEL_THREADS - 1; j >= 0; j--) {
            error = okn_syscall_futex_signal(futex1110_wait_tag[j]);
            assert(!error);
        }

        /* Wait for threads to finish. */
        okn_barrier(&futex1110_barrier);
    }

    /* Wait for the threads to exit */
    for (int i = 0; i < MAX_KERNEL_THREADS; i++) {
        okn_syscall_thread_join(tid[i]);
    }

    /* All done. */
    return 0;
}

/*
 * FUTEX1200 : Shared Variable / Barrier.
 *
 * Ensure that 5 threads updating a shared variable can do so
 * safely if it is protected by a lock.
 */

#define FUTEX1200_CHILDREN 3
#define FUTEX1200_ITERATIONS 10

static okn_futex_t futex1200_lock;
okn_barrier_t futex1200_barrier;
static volatile int futex1200_mem;

static void
futex1200_child(void *arg)
{
    /* Start. */
    okn_barrier(&futex1200_barrier);

    /* Increase the shared memory many times. */
    for (int i = 0; i < FUTEX1200_ITERATIONS; i++) {
        okn_futex_lock(&futex1200_lock);
        futex1200_mem++;
        okn_futex_unlock(&futex1200_lock);
    }

    /* All done. */
    okn_barrier(&futex1200_barrier);
}

TEST(FUTEX, 1200, "Shared variable / Barrier")
{
    int tids[FUTEX1200_CHILDREN];

    okn_futex_init(&futex1200_lock);
    futex1200_mem = 0;
    okn_barrier_init(&futex1200_barrier, FUTEX1200_CHILDREN + 1);

    /* Create threads. */
    for (int i = 0; i < FUTEX1200_CHILDREN; i++) {
        tids[i] = create_thread(futex1200_child, 1, NULL);
        assert(tids[i] >= 0);
    }

    /* Wait for them to start up. */
    okn_barrier(&futex1200_barrier);

    /* Wait for them to finish up. */
    okn_barrier(&futex1200_barrier);

    /* Ensure counter is correct. */
    assert(futex1200_mem == FUTEX1200_ITERATIONS * FUTEX1200_CHILDREN);

    /* wait for all children to exit */
    for (int i = 0; i < FUTEX1200_CHILDREN; i++) {
        okn_syscall_thread_join(tids[i]);
    }

    return 0;
}


/*
 * Futex Tests
 */
struct nano_test **
get_futex_tests(void)
{
    static struct nano_test *tests[] = {
        &FUTEX_1000,
        &FUTEX_1010,
        &FUTEX_1011,
        &FUTEX_1100,
        &FUTEX_1110,
        &FUTEX_1200,
        NULL
        };
    return tests;
}
