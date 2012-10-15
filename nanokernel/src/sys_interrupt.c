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
 * Neo Interrupt Handling.
 *
 * The current design is lock-free for interrupt delivery / interrupt waiting.
 * This comes as the cost of a few complexities: The main limitation is that
 * any thread may register for at most one interrupt. This is to prevent two
 * handlers from coming in at the same time trying to wake up the same thread
 * twice.
 *
 * We rely primarily on interrupt auto disable hardware as a
 * poor-man's locking device. See further down the file for more information.
 */

#include <nano.h>
#include <thread.h>
#include <schedule.h>
#include <serial.h>
#include <syscalls.h>
#include <interrupt.h>

/* Interrupt lock. */
spinlock_t interrupt_lock = SPIN_LOCK_INIT;

/* Registered handlers for each interrupt. */
tcb_t *interrupt_handlers[NUM_INTERRUPTS];

#if NUM_EXECUTION_UNITS > 1
/*
 * A priority to interrupt mask mapping.
 *
 * Threads may freely read this without taking the interrupt
 * lock, so writers must be careful to always keep it in a
 * consistant state.
 */
word_t priority_int_mask[NUM_PRIORITIES];
#endif

/*
 * Wait for the next incoming interrupt.
 */
ATTRIBUTE_NORETURN(sys_interrupt_wait);
NORETURN void
sys_interrupt_wait()
{
    /* Ensure we are registered to the interrupt. */
    if (EXPECT_FALSE(current_tcb->registered_interrupt == -1UL)) {
        syscall_return_error(-1, EINVAL);
    }

    /* Go to sleep. */
    tcb_t *next = deactivate_self_schedule(THREAD_STATE_WAIT_INT);

    /* Unmask our interrupt. */
    arch_unmask_interrupt(current_tcb->registered_interrupt);

    /* Done. */
    switch_to(next);
}

/*
 * Register the current thread for an interrupt.
 */
ATTRIBUTE_NORETURN(sys_interrupt_register);
NORETURN void
sys_interrupt_register(int interrupt)
{
    /* Take the interrupt lock. */
    spin_lock(&interrupt_lock);

    /* Ensure that the interrupt number is valid. */
    if (interrupt >= NUM_INTERRUPTS) {
        spin_unlock(&interrupt_lock);
        syscall_return_error(1, EINVAL);
    }

    /* Ensure nobody else has it yet. */
    if (interrupt_handlers[interrupt] != NULL) {
        spin_unlock(&interrupt_lock);
        syscall_return_error(1, EBUSY);
    }

    /* Ensure that we have not already registered an interrupt. */
    if (current_tcb->registered_interrupt != -1UL) {
        spin_unlock(&interrupt_lock);
        syscall_return_error(1, EBUSY);
    }

#if NUM_EXECUTION_UNITS > 1
    /* Determine the priority of the thread, and update the priority
     * masks to only mask out the interrupt */
    for (int i = NUM_PRIORITIES - 1; i > current_tcb->priority; i--) {
        priority_int_mask[i] |= INTERRUPT_BITMASK(interrupt);
    }
    for (int i = current_tcb->priority; i >= 0; i--) {
        priority_int_mask[i] &= ~INTERRUPT_BITMASK(interrupt);
    }
#endif

    /* Setup the handler. */
    interrupt_handlers[interrupt] = current_tcb;
    current_tcb->registered_interrupt = interrupt;

    /* Enable the interrupt. */
    arch_enable_interrupt(interrupt);

    spin_unlock(&interrupt_lock);
    syscall_return_success(0);
}

/*
 * Deregister the current thread for an interrupt.
 */
ATTRIBUTE_NORETURN(sys_interrupt_deregister);
NORETURN void
sys_interrupt_deregister(int interrupt)
{
    /* Take the interrupt lock. */
    spin_lock(&interrupt_lock);

    /* Ensure that the interrupt number is valid. */
    if (interrupt >= NUM_INTERRUPTS) {
        spin_unlock(&interrupt_lock);
        syscall_return_error(1, EINVAL);
    }

    /* Ensure that we currently hold it. */
    if (interrupt_handlers[interrupt] != current_tcb) {
        spin_unlock(&interrupt_lock);
        syscall_return_error(1, EINVAL);
    }

#if NUM_EXECUTION_UNITS > 1
    /* Threads need not accept this interrupt any longer. */
    for (int i = NUM_PRIORITIES - 1; i >= 0; i--) {
        priority_int_mask[i] |= INTERRUPT_BITMASK(interrupt);
    }
#endif

    /* Disable the interrupt. */
    arch_disable_interrupt(interrupt);
    current_tcb->registered_interrupt = -1UL;

    /* Record that this thread is no longer a handler. */
    interrupt_handlers[interrupt] = NULL;

    spin_unlock(&interrupt_lock);
    syscall_return_success(0);
}

void
sys_interrupt_pre_thread_delete_callback(tcb_t * tcb)
{
    /* Take the interrupt lock. */
    spin_lock(&interrupt_lock);

    if(tcb->registered_interrupt != -1UL) {
        /* Disable the interrupt. */
        arch_disable_interrupt(tcb->registered_interrupt);

        /* Record that this thread is no longer a handler. */
        interrupt_handlers[tcb->registered_interrupt] = NULL;
        tcb->registered_interrupt = -1UL;
    }

    spin_unlock(&interrupt_lock);
}
