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
 * Nano Test Miscellananous Functions
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <nano/nano.h>
#include <atomic_ops/atomic_ops.h>

#include <nanotest/nanotest.h>
#include <nanotest/system.h>
#include <nanotest/clock.h>

#define DEFAULT_STACK_SIZE 4096

/*
 * flush_timer_interrupt()
 *
 * Certain architectures have a problem where the processor's interrupts can
 * not be masked except by hardware when an interrupt actually comes in. This
 * means that the first interrupt received for each interrupt number will
 * always trigger a thread to receive the interrupt, regardless of whether or
 * not it has a handler associated with it.
 *
 * This function will cause one interrupt to trigger for the timer to avoid
 * these problems.
 */
void flush_timer_interrupt()
{
    printf("Testing interrupts and system clock");
    pause(1);
    printf("... ");
    clock_test();
    printf("done.\n");
}

/*
 * Thread entry point.
 */
static void
nanotest_thread_entry(uint32_t *args)
{
    ARCH_THREAD_INIT;

    /* Read off parameters. */
    void *stack = (void *)args[0];
    entry_point_t entry = (entry_point_t)args[1];
    void *user_arg = (void *)args[2];

    /* Call the user's entry point. */
    entry(user_arg);

    /* Free the stack, and delete ourself. This is a bit risky, as we
     * don't really want to write to the stack after we free it. We just
     * write the function calls, inspect the generated assembly, and
     * hope for the best. */
    free(stack);
    okn_syscall_thread_exit();
    for (;;);
}

/*
 * Create a new thread with the argument 'arg', and return the tid.
 *
 * Return negative tid on error.
 */
int
create_thread(entry_point_t entry, int prio, void *arg)
{
    /* Create a stack for the new thread. */
    void *stack = malloc(DEFAULT_STACK_SIZE);
    if (stack == NULL)
        return 1;

    /* Use the top of the stack to pass in arguments. */
    uint32_t *args = (uint32_t *)stack;
    args[0] = (uint32_t)stack;
    args[1] = (uint32_t)entry;
    args[2] = (uint32_t)arg;

    /* Try create the new thread. */
    int tid = okn_syscall_thread_create(nanotest_thread_entry,
            (char *)stack + DEFAULT_STACK_SIZE - 32, (unsigned long)stack,
            prio);
    if (tid < 0) {
        /* Error in the create. */
        free(stack);
        return tid;
    }

    /* Otherwise, we are done. */
    return tid;
}

/* Set to indicate to spinners that they should destroy themselves. */
static volatile int spinner_destroy = 0;

/* Number of spinners currently in the system. */
static int num_spinners = 0;

/* Spinner tids. */
static int spinner_tids[NUM_EXECUTION_UNITS];


/*
 * Entry point for spinner threads.
 */
static void
spinner_thread(void *arg)
{
    while (!spinner_destroy);
}

/*
 * Create 'n' threads designed to busy wait at the given priority.
 */
void create_spinners(int n, int priority)
{
    /* Allow spinners to live. */
    spinner_destroy = 0;

    /* Create the spinners. */
    for (int i = 0; i < n; i++) {
        int tid = create_thread(spinner_thread, priority, NULL);
        assert(tid >= 0);
        spinner_tids[i] = tid;
    }

    num_spinners = n;
}

/*
 * Delete all the spinners currently in the system.
 */
void delete_spinners(void)
{
    /* Inform spinner to die. */
    spinner_destroy = 1;

    /* Wait for the spinners to die. */
    for (int i = 0; i < num_spinners; i++)
        okn_syscall_thread_join(spinner_tids[i]);
    num_spinners = 0;
}

/*
 * Pause the current thread for the specified amount of time.
 */
void pause(unsigned int time)
{
    clock_wait(time);
}

