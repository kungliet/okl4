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
#include <ctest/ctest.h>
#include <atomic_ops/atomic_ops.h>

#include <okl4/pd.h>

#include "../helpers.h"

#define STACK_SIZE       1024
#define MAX_THREADS      64

static okl4_kthread_t *
create_pd_thread(void *ip, char **result_stack)
{
    okl4_kthread_t *result;
    int error;
    char *stack;
    char *stack_location;

    /* Create a stack. */
    stack = malloc(STACK_SIZE);
    assert(stack);
    stack_location = stack + STACK_SIZE;

    /* Create the thread. */
    okl4_pd_thread_create_attr_t attr;
    okl4_pd_thread_create_attr_init(&attr);
    okl4_pd_thread_create_attr_setspip(&attr,
            (okl4_word_t)stack_location, (okl4_word_t)ip);
    error = okl4_pd_thread_create(root_pd, &attr, &result);
    if (error) {
        free(stack);
        return NULL;
    }
    okl4_pd_thread_start(root_pd, result);

    *result_stack = stack;
    return result;
}

static void
join_pd_thread(okl4_kthread_t *thread, char *stack)
{
    int error;

    error = okl4_pd_thread_join(root_pd, thread);
    assert(!error);
    free(stack);
}

/*
 * Attempt to create a large number of threads and delete them again.
 */

static okl4_atomic_word_t pd0100_threads;

static void
child_entry_point(void)
{
    okl4_atomic_inc(&pd0100_threads);
    printf(".");
    okl4_pd_thread_exit();
}

/*
 * Create as many threads as possible (at most 'max_threads').
 * Return the number of threads made.
 */
static int
test_thread_creation(int max_threads)
{
    char **stacks;
    okl4_kthread_t **threads;
    int i;
    int num_threads;

    /* Setup memory. */
    stacks = malloc(sizeof(char *) * max_threads);
    assert(stacks);
    threads = malloc(sizeof(okl4_kthread_t *) * max_threads);
    assert(threads);

    /* Create threads. */
    num_threads = 0;
    okl4_atomic_init(&pd0100_threads, 0);
    for (i = 0; i < max_threads; i++) {
        threads[i] = create_pd_thread(child_entry_point, &stacks[i]);
        if (threads[i] == NULL) {
            break;
        }
        num_threads++;
    }

    /* Join on threads. */
    for (i = 0; i < num_threads; i++) {
        join_pd_thread(threads[i], stacks[i]);
    }

    /* Ensure that each thread ran. */
    assert(okl4_atomic_read(&pd0100_threads) == num_threads);
    free(stacks);
    free(threads);
    return num_threads;
}

/*
 * Create a single thread using the Nano PD interface, and then join it. Ensure
 * the thread runs.
 */
START_TEST(PD0100)
{
    int i;

    /* Simple test of one thread. */
    for (i = 0; i < 10; i++) {
        int created = test_thread_creation(1);
        assert(created == 1);
        (void)created;
    }
    printf("\n");
}
END_TEST

/*
 * Create a large number of threads, exhausting memory. Ensure that at least
 * two threads could be created, and all created threads ran. When all threads
 * have been deleted, ensure that the same number of threads may be created
 * again.
 */
START_TEST(PD0200)
{
    int num_threads_a;
    int num_threads_b;

    /* Simple test of one thread. */
    num_threads_a = test_thread_creation(512);
    assert(num_threads_a >= 2);
    printf("\n");

    /* Simple test of one thread. */
    num_threads_b = test_thread_creation(512);
    assert(num_threads_a == num_threads_b);
    printf("\n");
}
END_TEST

TCase *
make_pd_tcase(void)
{
    TCase * tc;

    tc = tcase_create("Nano Protection Domains");
    tcase_add_test(tc, PD0100);
    tcase_add_test(tc, PD0200);

    return tc;
}

