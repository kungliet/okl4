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
 * Helper functions for threaded tests.
 */

#include <stdio.h>
#include <ctest/ctest.h>

#include <okl4/message.h>
#include <okl4/kthread.h>
#include <okl4/init.h>
#include <okl4/pd.h>

#if defined(OKL4_KERNEL_MICRO)
#include <l4/misc.h>
#endif

#include "helpers.h"

#define STACK_SIZE 4096

/* Startup arguments passed to children. */
struct startup_args
{
    okl4_word_t id;
    okl4_kcap_t *peers;
    test_child_fn startup;
    okl4_kcap_t parent;
    void *arg;
};

/* Child startup point. */
static void
child_startup(void)
{
    struct startup_args args;
    okl4_word_t bytes;
    int error;

    /* Setup libokl4. */
    okl4_init_thread();

    /* Wait for parent to tell us our args. */
    error = okl4_message_wait(&args, sizeof(args), &bytes, NULL);
    assert(!error);
    assert(bytes == sizeof(args));
    error = okl4_message_send(args.parent, NULL, 0);
    assert(!error);

    /* Run the child function. */
    args.startup(args.id, args.peers, args.arg);

    /* Tell our parent we have finished. */
    error = okl4_message_send(args.parent, NULL, 0);
    assert(!error);

    /* Block waiting for them to kill us. */
#if defined(OKL4_KERNEL_MICRO)
    L4_WaitForever();
#else
    okl4_kthread_exit();
#endif
}

/* Test function. */
void
multithreaded_test(okl4_word_t num_threads,
        test_child_fn entry_point, void *arg)
{
    okl4_word_t i;
    okl4_kthread_t **threads;
    okl4_kcap_t *caps;
    okl4_word_t **stacks;
    int error;

    /* Create memory for threads and stacks. */
    threads = malloc(sizeof(okl4_kthread_t *) * num_threads);
    assert(threads);
    caps = malloc(sizeof(okl4_kcap_t) * num_threads);
    assert(caps);
    stacks = malloc(sizeof(okl4_word_t *) * num_threads);
    assert(stacks);

    /* Create threads. */
    for (i = 0; i < num_threads; i++) {
        okl4_pd_thread_create_attr_t attr;

        /* Create stack. */
        stacks[i] = malloc(STACK_SIZE);
        assert(stacks[i]);

        /* Create the threads. */
        okl4_pd_thread_create_attr_init(&attr);
        okl4_pd_thread_create_attr_setspip(&attr,
                (okl4_word_t)stacks[i] + STACK_SIZE,
                (okl4_word_t)child_startup);
        error = okl4_pd_thread_create(root_pd, &attr, &threads[i]);
        assert(!error);
        caps[i] = okl4_kthread_getkcap(threads[i]);
        okl4_pd_thread_start(root_pd, threads[i]);
    }

    /* Wake the threads up. */
    for (i = 0; i < num_threads; i++) {
        struct startup_args args;
        okl4_word_t bytes;
        args.id = i;
        args.peers = caps;
        args.startup = entry_point;
        args.parent = okl4_kthread_getkcap(root_thread);
        args.arg = arg;
        error = okl4_message_call(caps[i], &args, sizeof(args), NULL, 0, &bytes);
        assert(!error);
    }

    /* Wait for threads to finish. */
    for (i = 0; i < num_threads; i++) {
        error = okl4_message_wait(NULL, 0, NULL, NULL);
        assert(!error);
    }

    /* Delete all the threads, and free memory. */
    for (i = 0; i < num_threads; i++) {
#if defined(OKL4_KERNEL_MICRO)
        okl4_pd_thread_delete(root_pd, threads[i]);
#else
        error = okl4_pd_thread_join(root_pd, threads[i]);
        assert(!error);
#endif
        free(stacks[i]);
    }
    free(threads);
    free(caps);
    free(stacks);
}

