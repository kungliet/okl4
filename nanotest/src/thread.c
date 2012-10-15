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

/* Wait for another thread to signal(). */
inline static void
wait(int n)
{
    int error = okn_syscall_futex_wait(n + 1);
    assert(!error);
    (void) error;
}

/* Signal a thread in a wait() state. */
inline static void
signal(int n)
{
    int error = okn_syscall_futex_signal(n + 1);
    assert(!error);
    (void) error;
}

/*
 * TEST1000 : Simple Thread Create
 *
 * Attempt to create a new thread, and wait for it to run.
 */

static volatile int test1000_child_pid;

static void
test1000_child_thread(void *arg)
{
    test1000_child_pid = okn_syscall_thread_myself();
    signal(0);
}

TEST(THREAD, 1000, "Simple thread create")
{
    /* Create a new thread. */
    int tid = create_thread(test1000_child_thread, 1, NULL);
    assert(tid >= 0);

    /* Wait for it to run. */
    wait(0);

    /* Ensure it has a different PID to us. */
    assert(test1000_child_pid != okn_syscall_thread_myself());

    /* Ensure the pid reported back to us is the same as its pid. */
    assert(tid == test1000_child_pid);

    /* wait for it to exit */
    okn_syscall_thread_join(tid);

    return 0;
}

/*
 * TEST1010 : Many Thread Create
 *
 * Create many new threads, until out of memory.
 */

static okn_semaphore_t test1010_wake;
static okn_semaphore_t test1010_finished;

#define THREAD1010_INITIAL_BUFFER_SIZE 32

static void
test1010_child_thread(void *arg)
{
    okn_semaphore_down(&test1010_wake);
    okn_semaphore_up(&test1010_finished);
}

TEST(THREAD, 1010, "Many thread create")
{
    int threads_made = 0;
    int buffer_size = THREAD1010_INITIAL_BUFFER_SIZE;
    int * tids = (int *)malloc(buffer_size*sizeof(int));
    assert(tids);

    okn_semaphore_init(&test1010_wake, 0);
    okn_semaphore_init(&test1010_finished, 0);

    /* Create new threads until we run out of memory. */
    while (1) {
        tids[threads_made] = create_thread(test1010_child_thread,
                ROOT_TASK_PRIORITY + 1, NULL);
        if (tids[threads_made] < 0)
            break;
        threads_made++;

        /* resize the thread id buffer if necessary */
        if(threads_made == buffer_size) {
            buffer_size *= 2;
            tids = realloc(tids, buffer_size*sizeof(int));
            assert(tids);
        }
    }
    assert(threads_made > 1);

    /* Allow the threads to die. */
    for (int i = 0; i < threads_made; i++)
        okn_semaphore_up(&test1010_wake);
    for (int i = 0; i < threads_made; i++)
        okn_semaphore_down(&test1010_finished);
    for (int i = 0; i < threads_made; i++)
        okn_syscall_thread_join(tids[i]);

    /* Create the same number of threads again. */
    for (int i = 0; i < threads_made; i++) {
        tids[i] = create_thread(test1010_child_thread,
                ROOT_TASK_PRIORITY + 1, NULL);
        assert(tids[i] >= 0);
    }

    /* Allow the threads to die. */
    for (int i = 0; i < threads_made; i++)
        okn_semaphore_up(&test1010_wake);
    for (int i = 0; i < threads_made; i++)
        okn_semaphore_down(&test1010_finished);
    for (int i = 0; i < threads_made; i++)
        okn_syscall_thread_join(tids[i]);

    /* free the buffer */
    free(tids);
    return 0;
}

/*
 * TEST1100 : Thread Yield
 *
 * Create 10 threads, and wait for them all to run a few times.
 */

#define TEST1100_ITERATIONS    10
#define TEST1100_CHILD_THREADS 5
okl4_atomic_word_t test1100_count;
okn_semaphore_t test1100_sem;

static void
test1100_child_thread(void *args)
{
    /* Increase the counter. */
    for (int i = 0; i < TEST1100_ITERATIONS; i++) {
        okl4_atomic_inc(&test1100_count);
        okn_syscall_yield();
    }

    /* Keep yielding until all threads have incremented the counter
     * by their allocation. */
    while (okl4_atomic_read(&test1100_count)
            != TEST1100_ITERATIONS * TEST1100_CHILD_THREADS) {
        okn_syscall_yield();
    }

    /* Let parent know we are done. */
    okn_semaphore_up(&test1100_sem);
}

TEST(THREAD, 1100, "Thread yield")
{
    int tids[TEST1100_CHILD_THREADS];

    okn_semaphore_init(&test1100_sem, 0);

    /* Set the counter to 0. */
    okl4_atomic_set(&test1100_count, 0);

    /* Create children threads. */
    for (int i = 0; i < TEST1100_CHILD_THREADS; i++) {
        tids[i] = create_thread(test1100_child_thread, 1, NULL);
        assert(tids[i] >= 0);
    }

    /* Wait for all children to finish. */
    for (int i = 0; i < TEST1100_CHILD_THREADS; i++)
        okn_semaphore_down(&test1100_sem);

    /* Wait for all children to exit */
    for (int i = 0; i < TEST1100_CHILD_THREADS; i++) {
        okn_syscall_thread_join(tids[i]);
    }

    return 0;
}

/* Test thread join */
static void
test1200_child(void * args)
{
    (void) args;
}

TEST(THREAD, 1200, "Thread join")
{
    /* create and join a thread */
    int tid = create_thread(test1200_child, 0, NULL);
    int error = okn_syscall_thread_join(tid);
    assert(!error);

    /* join a thread that has already been joined */
    error = okn_syscall_thread_join(tid);
    assert(error);

    /* join a non-existant tid */
    error = okn_syscall_thread_join(-1);
    assert(error);

    return 0;
}


/*
 * Thread tests
 */
struct nano_test **
get_thread_tests(void)
{
    static struct nano_test *tests[] = {
        &THREAD_1000,
        &THREAD_1010,
        &THREAD_1100,
        &THREAD_1200,
        NULL
        };
    return tests;
}

