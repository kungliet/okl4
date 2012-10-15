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
 * Nano Interrupt Tests
 */

#include <stdio.h>
#include <stdlib.h>

#include <nano/nano.h>

#include <nanotest/nanotest.h>
#include <nanotest/system.h>
#include <nanotest/clock.h>

/*
 * INT1000 : Register/Deregister for interrupt.
 *
 * Ensure we can register for an interrupt, and then register for it again.
 */
TEST(INT, 1000, "Register/Deregister for interrupt")
{
    int error;

    /* Register/deregister for an interrupt. */
    for (int i = 0; i < 3; i++) {
        error = okn_syscall_interrupt_register(0);
        assert(!error);
        error = okn_syscall_interrupt_deregister(0);
        assert(!error);
    }

    return 0;
}

/*
 * INT1001 : Register for two interrupts
 *
 * Ensure we cannot register for two interrupts simultaneously
 */
TEST(INT, 1001, "Register for two interrupts")
{
    int error;

    /* Register/deregister for an interrupt. */
    error = okn_syscall_interrupt_register(0);
    assert(!error);
    error = okn_syscall_interrupt_register(1);
    assert(error);
    error = okn_syscall_interrupt_deregister(0);
    assert(!error);

    return 0;
}

/*
 * INT1002 : Two threads register for same interupt
 *
 * Ensure two threads cannot register the same interrupt simultaneously
 */

static void
int1002_child(void * args) 
{
    /* try to register for an already registered interrupt */
    int error = okn_syscall_interrupt_register((int)args);
    assert(error);
    /* try to deregister someone elses interrupt */
    error = okn_syscall_interrupt_deregister((int)args);
    assert(error);
}

TEST(INT, 1002, "Two threads register for one interrupts")
{
    int error, tid;

    /* Register for an interrupt. */
    error = okn_syscall_interrupt_register(0);
    assert(!error);

    /* create child to run test */
    tid = create_thread(int1002_child, 0, (void *)0);
    assert(tid >= 0);

    /* wait for child to run test */
    error = okn_syscall_thread_join(tid);
    assert(!error);

    /* deregister for the interrupt */
    error = okn_syscall_interrupt_deregister(0);
    assert(!error);

    return 0;
}

/*
 * INT1003 : Thread exit deregisters interrupt
 *
 * Ensure thread exit by a thread that is registered for an interrupt 
 * deregisters the interrupt
 */

static void
int1003_child(void * args) 
{
    int error;

    /* test thread_exit when registered for an interrupt */
    error = okn_syscall_interrupt_register((int)args);
    assert(!error);
}

TEST(INT, 1003, "Thread exit deregisters interrupt")
{
    int error, tid;

    /* create child to run test */
    tid = create_thread(int1003_child, 0, (void *)0);
    assert(tid >= 0);

    /* wait for child to register and exit */
    error = okn_syscall_thread_join(tid);
    assert(!error);

    /* test that we can now register for the interrupt */
    error = okn_syscall_interrupt_register(0);
    assert(!error);

    /* deregister for the interrupt */
    error = okn_syscall_interrupt_deregister(0);
    assert(!error);

    return 0;
}

/*
 * INT1100 : Wait for an interrupt to fire.
 *
 * Ensure we can register for an interrupt, and then register for it again.
 */

#define INT1100_ITERATIONS 10

static okn_semaphore_t int1100_sem;

static void
int1100_child(void *arg)
{
    okn_semaphore_up(&int1100_sem);

    /* Wait for the interrupt. */
    for (int i = 0; i < INT1100_ITERATIONS; i++) {
        clock_wait(1);
        okn_semaphore_up(&int1100_sem);
    }

}

TEST(INT, 1100, "Wait for an interrupt to fire.")
{
    okn_semaphore_init(&int1100_sem, 0);

    /* Create a child thread. */
    int tid = create_thread(int1100_child, ROOT_TASK_PRIORITY + 1, NULL);
    assert(tid >= 0);

    /* Fire 10 interrupts. */
    for (int i = 0; i < INT1100_ITERATIONS; i++) {
        okn_semaphore_down(&int1100_sem);
    }
    okn_semaphore_down(&int1100_sem);

    /* Wait for the child thread to exit */
    okn_syscall_thread_join(tid);

    return 0;
}

/*
 * Interrupt Tests
 */
struct nano_test **
get_interrupt_tests(void)
{
    static struct nano_test *tests[] = {
        &INT_1000,
        &INT_1001,
        &INT_1002,
        &INT_1003,
        &INT_1100,
        NULL
        };
    return tests;
}

