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


#include <nano.h>
#include <thread.h>
#include <interrupt.h>
#include <schedule.h>
#include <serial.h>
#include <syscalls.h>

/* First free TCB. */
MASHED tcb_t *tcb_free_head;

/* Maximum number of threads in the system. (Patched at mash-time.) */
MASHED const unsigned int max_tcbs;

/* Address of the first TCB. (Patched at mash-time.) */
MASHED tcb_t * const tcbs;

/* Lock to protect TCBs in the free list. */
static spinlock_t tcb_lock = SPIN_LOCK_INIT;

/* Callbacks */
#if 0
#define CALLBACK(x, args...)                                          \
    do {                                                              \
        extern void x(tcb_t *);                                       \
        x(args);                                                      \
    } while (0)
#endif

/* Callbacks to run prior to creating a thread. */
INLINE static void
pre_create_thread_callbacks(tcb_t *tcb)
{
    sys_signal_pre_thread_create_callback(tcb);
    sys_ipc_pre_thread_create_callback(tcb);
    //CALLBACK(sys_signal_pre_thread_create_callback, tcb);
    //CALLBACK(sys_ipc_pre_thread_create_callback, tcb);
}

/* Callbacks to run after creating a thread. */
INLINE static void
post_create_thread_callbacks(tcb_t *tcb)
{
    sys_ipc_post_thread_create_callback(tcb);
    sys_signal_post_thread_create_callback(tcb);
    //CALLBACK(sys_ipc_post_thread_create_callback, tcb);
    //CALLBACK(sys_signal_post_thread_create_callback, tcb);
}

/* Callbacks to run before deleting a thread. */
INLINE static void
pre_delete_thread_callbacks(tcb_t *tcb)
{
    sys_signal_pre_thread_delete_callback(tcb);
    sys_ipc_pre_thread_delete_callback(tcb);
    sys_interrupt_pre_thread_delete_callback(tcb);
    //CALLBACK(sys_signal_pre_thread_delete_callback, tcb);
    //CALLBACK(sys_ipc_pre_thread_delete_callback, tcb);
}

/* Callbacks to run after delting a thread. */
INLINE static void
post_delete_thread_callbacks(tcb_t *tcb)
{
    sys_ipc_post_thread_delete_callback(tcb);
    sys_signal_post_thread_delete_callback(tcb);
    //CALLBACK(sys_ipc_post_thread_delete_callback, tcb);
    //CALLBACK(sys_signal_post_thread_delete_callback, tcb);
}

ATTRIBUTE_NORETURN(sys_thread_create);
NORETURN void
sys_thread_create(word_t pc, word_t sp, word_t r0, word_t prio)
{
    /* Is the priority correct? */
    if (EXPECT_FALSE(prio > MAX_PRIORITY))
        syscall_return_error(-1, EINVAL);

    spin_lock(&tcb_lock);

    /* Are there any free TCBs present? */
    if (EXPECT_FALSE(tcb_free_head == NULL)) {
        spin_unlock(&tcb_lock);
        syscall_return_error(-1, ENOMEM);
    }

    /* Dequeue the first TCB in the list. */
    tcb_t *new_tcb = tcb_free_head;
    if (new_tcb->next == new_tcb) {
        tcb_free_head = NULL;
    } else {
        tcb_t *next = new_tcb->next;
        tcb_t *prev = new_tcb->prev;
        tcb_free_head = next;
        next->prev = prev;
        prev->next = next;
    }
    new_tcb->next = NULL;
    new_tcb->prev = NULL;

    ASSERT(new_tcb->thread_state == THREAD_STATE_HALTED);

    /* Setup its context frame. */
    new_tcb->sp = sp;
    new_tcb->pc = pc;
    new_tcb->r0 = r0;
    new_tcb->priority = prio;

    /* Enqueue the thread as runnable. */
    pre_create_thread_callbacks(new_tcb);
    tcb_t *next = activate_schedule(new_tcb);
    post_create_thread_callbacks(new_tcb);

    spin_unlock(&tcb_lock);

    /* Switch to the highest thread, returning the new thread's tid. */
    set_syscall_return_val_success(current_tcb, new_tcb->tid);
    switch_to(next);
}

/*
 * Reap a thread from the zombie (waiting for join) state 
 * to the deleted state
 */
static void
thread_reap(tcb_t * tcb)
{
    ASSERT(tcb->thread_state == THREAD_STATE_ZOMBIE);

    /* thread subsystem owns both zombie and halted state
     * so no need to call the scheduler
     */
    tcb->thread_state = THREAD_STATE_HALTED;

    /* Enqueue tcb onto the free list. */
    if (tcb_free_head == NULL) {
        tcb_free_head = tcb;
        tcb->next = tcb;
        tcb->prev = tcb;
    } else {
        tcb_t *next = tcb_free_head;
        tcb_t *prev = tcb_free_head->prev;
        prev->next = tcb;
        next->prev = tcb;
        tcb->next = next;
        tcb->prev = prev;
        tcb_free_head = tcb;
    }
}

/*
 * Delete the thread.
 */
ATTRIBUTE_NORETURN(sys_thread_exit);
NORETURN void
sys_thread_exit(void)
{
    spin_lock(&tcb_lock);

    /* Tell the scheduler to deactivate us. */
    pre_delete_thread_callbacks(current_tcb);
    deactivate_self(THREAD_STATE_ZOMBIE);
    post_delete_thread_callbacks(current_tcb);

    /* Wake up anyone joining on us. */
    if (current_tcb->joiner) {
        thread_reap(current_tcb);
        activate(current_tcb->joiner);
        current_tcb->joiner = NULL;
    }

    spin_unlock(&tcb_lock);

    /* Find someone else to run. */
    tcb_t *next = schedule();
    switch_to(next);
}

/*
 * Wait for a thread to exit.
 */
ATTRIBUTE_NORETURN(sys_thread_join);
NORETURN void
sys_thread_join(word_t thread_id)
{
    /* Does the thread exist? */
    if (EXPECT_FALSE(thread_id >= max_tcbs))
        syscall_return_error(1, EINVAL);

    /* Find the destination thread. */
    tcb_t *dest = &tcbs[thread_id];

    spin_lock(&tcb_lock);

    /* Is the thread alive? */
    if (EXPECT_FALSE(dest->thread_state == THREAD_STATE_HALTED)) {
        spin_unlock(&tcb_lock);
        syscall_return_error(1, EINVAL);
    }

    /* Is the thread already in zombie state? */
    if (EXPECT_TRUE(dest->thread_state == THREAD_STATE_ZOMBIE)) {
        thread_reap(dest);
        spin_unlock(&tcb_lock);
        syscall_return_success(0);
    }

    /* Only one thread is allowed to join a thread */
    if (EXPECT_FALSE(dest->joiner)) {
        spin_unlock(&tcb_lock);
        syscall_return_error(1, EINVAL);
    }

    /* Dequeue ourselves. */
    set_syscall_return_val_success(current_tcb, 0);
    tcb_t *next = deactivate_self_schedule(THREAD_STATE_WAIT_JOIN);

    /* Set ourselves up as dest's joiner */
    dest->joiner = current_tcb;

    /* Done. */
    spin_unlock(&tcb_lock);
    switch_to(next);
}

/*
 * Get a thread's unique identifier.
 */
ATTRIBUTE_NORETURN(sys_thread_exit);
NORETURN void
sys_thread_myself(void)
{
    syscall_return_success(current_tcb->tid);
}

