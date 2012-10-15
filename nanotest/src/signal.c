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

/*
 * SIGNAL1000 : Signal self
 *
 * Send ourself a signal, and make sure we receive it.
 */
TEST(SIGNAL, 1000, "Signal self")
{
    int error;
    unsigned int result;

    /* Flush signals. */
    okn_syscall_signal_poll(~0UL);

    /* Send self a signal. */
    error = okn_syscall_signal_send(okn_syscall_thread_myself(), 0xdeadbeef);
    assert(!error);

    /* Ensure we got it. */
    result = okn_syscall_signal_poll(~0UL);
    assert(result == 0xdeadbeef);

    /* Ensure flushed out. */
    result = okn_syscall_signal_poll(~0UL);
    assert(result == 0);

    /* Send self another signal */
    error = okn_syscall_signal_send(okn_syscall_thread_myself(), 0xc0decafe);
    assert(!error);

    /* Wait for the signal. */
    result = okn_syscall_signal_wait(0xffffffff);
    assert(result == 0xc0decafe);

    /* Ensure flushed out. */
    result = okn_syscall_signal_poll(~0UL);
    assert(result == 0);

    /* Send self a signal. */
    error = okn_syscall_signal_send(okn_syscall_thread_myself(), 0x00001111);
    assert(!error);

    /* Ensure that poll only returns the signals we mask on. */
    result = okn_syscall_signal_poll(0x0000000f);
    assert(result = 0x00000001);
    result = okn_syscall_signal_wait(0x000000ff);
    assert(result = 0x00000010);
    result = okn_syscall_signal_poll(0x00000fff);
    assert(result = 0x00000100);
    result = okn_syscall_signal_wait(0x0000ffff);
    assert(result = 0x00001000);

    return 0;
}

/*
 * SIGNAL1010 : Signal deleted thread
 *
 * Sends a signal to a deleted thread.
 */
TEST(SIGNAL, 1010, "Signal deleted thread")
{
    int error;

    /* Ensure threads from previous tests have had a chance
     * to die. */
    relax();

    /* Signal non-existant thread. */
    int myself = okn_syscall_thread_myself();
    error = okn_syscall_signal_send(myself + 1, 1);
    assert(error);

    return 0;
}

/*
 * SIGNAL1100 : Signal other thread
 *
 * Send ourself a signal, and make sure we receive it.
 */

static volatile unsigned int test1100_pid;
static volatile unsigned int test1100_ready;

static void
test1100_child(void *arg)
{
    int error;

    /* Give parent some time to sleep. */
    relax();

    /* Signal our parent with a signal they are not waiting on. */
    error = okn_syscall_signal_send(test1100_pid, 2);
    assert(!error);
    relax();

    /* Inform parent they are now allowed to wake. */
    test1100_ready = 1;

    /* Send parent the correct signal. */
    error = okn_syscall_signal_send(test1100_pid, 1);
    assert(!error);
}

TEST(SIGNAL, 1100, "Signal other thread")
{
    test1100_pid = okn_syscall_thread_myself();
    test1100_ready = 0;

    /* Create a child thread. */
    int tid = create_thread(test1100_child, 1, NULL);
    assert(tid >= 0);

    /* Wait for a signal. */
    unsigned int result = okn_syscall_signal_wait(1);

    /* Ensure the child actually signalled. */
    assert(test1100_ready);

    /* Ensure signal was correct. */
    assert(result == 1);
    (void) result;

    /* Ensure we also received the second sigunal. */
    result = okn_syscall_signal_poll(2);
    assert(result == 2);
    
    /* Wait for child to exit. */
    okn_syscall_thread_join(tid);

    return 0;
}

/*
 * Thread tests
 */
struct nano_test **
get_signal_tests(void)
{
    static struct nano_test *tests[] = {
        &SIGNAL_1000,
        &SIGNAL_1010,
        &SIGNAL_1100,
        NULL
        };
    return tests;
}

