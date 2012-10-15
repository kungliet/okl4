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
 * Nano Signal Tests
 */

#include <stdio.h>
#include <stdlib.h>

#include <nano/nano.h>
#include <atomic_ops/atomic_ops.h>

#include <nanotest/nanotest.h>
#include <nanotest/system.h>
#include <nanotest/performance.h>

/* Send an IPC message with a dummy payload. */
static unsigned long
send_null_ipc(unsigned long destination, unsigned long operation)
{
    return okn_syscall_ipc_send(
            1, 2, 3, 4, 5, 6, 7,
            destination, operation);
}

/* Receive an IPC message, discarding the payload. */
static unsigned long
recv_null_ipc(unsigned long src, unsigned long operation)
{
    unsigned long m1, m2, m3, m4, m5, m6, m7, sender;
    sender = ~0UL;

    int result = okn_syscall_ipc_recv(
            &m1, &m2, &m3, &m4, &m5, &m6, &m7, &sender,
            src, operation);
    assert(m1 == 1 && m2 == 2 && m3 == 3 && m4 == 4
            && m5 == 5 && m6 == 6 && m7 == 7 && sender != ~0UL);
    return result;
}

/*
 * IPC1000 : Send to invalid thread
 *
 * Send an IPC to an invalid thread.
 */
TEST(IPC, 1000, "Send to invalid thread")
{
    int error = send_null_ipc(-1, 0);
    assert(error);
    (void) error;

    return 0;
}

/*
 * IPC1010 : Non-blocking send to self
 *
 * Send a non-blocking IPC to ourself. The IPC should fail.
 */
TEST(IPC, 1010, "Non-blocking send to self")
{
    int error = send_null_ipc(okn_syscall_thread_myself(),
            OKN_IPC_NON_BLOCKING);
    assert(error);
    (void) error;

    return 0;
}

/*
 * IPC1020 : Multiple senders
 *
 * Receive messages from multiple blocked senders
 */
#define IPC1020_NUM_SENDERS 8

static void
ipc1020_sender (void * args)
{
    send_null_ipc((int)args, 0);
}

TEST(IPC, 1020, "Multiple senders")
{
    int tids[IPC1020_NUM_SENDERS];
    int myself = okn_syscall_thread_myself();
    int i;

    /* create sender threads at higher priority
     * so they send and block
     */
    for(i = 0; i < IPC1020_NUM_SENDERS; i++) {
        tids[i] = create_thread(ipc1020_sender, 20, (void *)myself);
        assert(tids[i] >= 0);
    }

    /* receive all of the messages */
    for(i = 0; i < IPC1020_NUM_SENDERS; i++) {
        recv_null_ipc(tids[i], 0);
    }

    /* wait for all the children to finish */
    for(i = 0; i < IPC1020_NUM_SENDERS; i++) {
        okn_syscall_thread_join(tids[i]);
    }

    return 0;
}

/*
 * IPC2000 : Send/Recv ping pong
 */
okn_semaphore_t test2000_sem;

#define TEST2000_ITERATIONS 100

static void
test2000_child(void *arg)
{
    int parent = (int)arg;
    okn_semaphore_up(&test2000_sem);

    for (int i = 0; i < TEST2000_ITERATIONS; i++) {
        recv_null_ipc(parent, 0);
        send_null_ipc(parent, 0);
    }
}

TEST(IPC, 2000, "Send/Recv ping pong")
{
    int child;

    okn_semaphore_init(&test2000_sem, 0);

    /* Start up a thread. */
    child = create_thread(test2000_child, 1, NULL);

    /* Wait for child to become ready. */
    okn_semaphore_down(&test2000_sem);

    /* Ping pong. */
    for (int i = 0; i < TEST2000_ITERATIONS; i++) {
        send_null_ipc(child, 0);
        recv_null_ipc(child, 0);
    }

    /* Wait for the child to exit. */
    okn_syscall_thread_join(child);

    /* Done. */
    return 0;
}

/*
 * IPC2010 : Call ping pong
 */
okn_semaphore_t test2010_sem;

#define TEST2010_ITERATIONS 10

static void
test2010_child(void *arg)
{
    int parent = (int) arg;
    okn_semaphore_up(&test2010_sem);

    recv_null_ipc(parent, 0);
    for (int i = 0; i < TEST2010_ITERATIONS - 1; i++) {
        send_null_ipc(parent, OKN_IPC_CALL);
    }
    send_null_ipc(parent, 0);
}

TEST(IPC, 2010, "Call ping pong")
{
    int child;
    okn_semaphore_init(&test2010_sem, 0);

    /* Start up a thread. */
    child = create_thread(test2010_child, 1, (void *)okn_syscall_thread_myself());

    /* Wait for child. */
    okn_semaphore_down(&test2010_sem);

    /* Ping pong. */
    for (int i = 0; i < TEST2010_ITERATIONS; i++) {
        send_null_ipc(child, OKN_IPC_CALL);
    }

    /* Wait for child to exit. */
    okn_syscall_thread_join(child);

    /* Done. */
    return 0;
}

/*
 * IPC tests
 */
struct nano_test **
get_ipc_tests(void)
{
    static struct nano_test *tests[] = {
        &IPC_1000,
        &IPC_1010,
        &IPC_1020,
        &IPC_2000,
        &IPC_2010,
        NULL
        };
    return tests;
}

