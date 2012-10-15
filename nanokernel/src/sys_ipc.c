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
 * IPC Support.
 *
 * This code is more proof-of-concept code, rather than a serious
 * communications mechanism. For single-address space communication, users are
 * better off using a producer/consumer type message buffer using futexes,
 * rather than IPC which (i) only supports small message sizes; (ii) requires
 * marshalling memory multiple times before reaching the destination; and (iii)
 * uses a non-standard ABI to achieve this.
 */

#include <nano.h>

#include <thread.h>
#include <schedule.h>
#include <serial.h>
#include <syscalls.h>

#define IPC_WAIT_ANY   (~0UL)
#define IPC_ERROR      (~0UL)

#define OPERATION_IS_NON_BLOCKING(x)  (((x) & (1 << 0)) != 0)
#define OPERATION_IS_CALL(x)          (((x) & (1 << 1)) != 0)

static spinlock_t ipc_lock = SPIN_LOCK_INIT;

/* Is the given thread 'dest' able to receive from 'src'? */
static int
is_ready_to_receive(tcb_t *src, tcb_t *dest)
{
    /* If they are not waiting for an IPC, we can't send. */
    if (dest->thread_state != THREAD_STATE_WAIT_IPC_RECV)
        return 0;

    /* Otherwise we can send if they are waiting for us or for anybody. */
    return dest->ipc_waiting_for == src || dest->ipc_waiting_for == NULL;
}

/* Copy message from 'src' to 'dest'. */
static void
copy_message(tcb_t *src, tcb_t *dest)
{
    /* The destination will need to restore its full context when it
     * wakes up. */
    dest->full_context_saved = 1;

    /* Copy across the message. */
    word_t * restrict s = (word_t * restrict)(void *)&src->r0;
    word_t * restrict d = (word_t * restrict)(void *)&dest->r0;
    d[1] = s[1];
    d[2] = s[2];
    d[3] = s[3];
    d[4] = s[4];
    d[5] = s[5];
    d[6] = s[6];
    d[7] = s[7];
}

/* Enqueue a the thread 'src' onto the send queue of thread 'dest'. */
static void
enqueue_send(tcb_t *dest, tcb_t *src)
{
    ASSERT(src->next == NULL && src->prev == NULL);

    if (dest->ipc_send_head == NULL) {
        /* Enqueue to front. */
        dest->ipc_send_head = src;
        src->next = src;
        src->prev = src;
    } else {
        /* Enqueue on rear. */
        tcb_t *first = dest->ipc_send_head;
        ASSERT(first->next != NULL);
        ASSERT(first->prev != NULL);
        tcb_t *last = first->prev;

        src->next = first;
        src->prev = last;
        first->prev = src;
        last->next = src;
    }
}

/* Dequeue the thread 'src' from the send queue of 'dest'. */
static void
dequeue_send(tcb_t *dest, tcb_t *src)
{
    ASSERT(src->next != NULL && src->prev != NULL);
    ASSERT(dest->ipc_send_head != NULL);
    ASSERT(dest->ipc_send_head->next != NULL);

    if (src->next == src) {
        /* Last thread on the queue. */
        dest->ipc_send_head = NULL;
    } else {
        /* Otherwise, pry it out. */
        src->next->prev = src->prev;
        src->prev->next = src->next;
        if (dest->ipc_send_head == src)
            dest->ipc_send_head = src->next;
    }
    src->next = NULL;
    src->prev = NULL;
}

/* Send an IPC to another thread. Return the thread that should be next
 * scheduled. */
static tcb_t *
do_send(tcb_t *dest, int is_call, int is_non_blocking)
{
    /* If the thread is not ready to receive, go to sleep or abort the call. */
    if (EXPECT_FALSE(!is_ready_to_receive(current_tcb, dest)))
        goto not_ready;

    /* Otherwise, copy our message across. */
    copy_message(current_tcb, dest);
    dest->ipc_waiting_for = NULL;
    set_syscall_return_val_success(dest, current_tcb->tid);

    /* If we are not performing a call, just wake up the destination and we are
     * done. */
    if (!is_call) {
        set_syscall_return_val_success(current_tcb, 0);
        return activate_schedule(dest);
    }

    /* Otherwise, wake up the destination and go to sleep. */
    current_tcb->ipc_waiting_for = dest;
    activate(dest);
    deactivate_self(THREAD_STATE_WAIT_IPC_RECV);
    return schedule();

    /* Destination thread not ready. */
not_ready:;
    if (is_non_blocking) {
        /* Return an error and keep running. */
        set_syscall_return_val_error(current_tcb, IPC_ERROR, EWOULDBLOCK);
        return current_tcb;
    }

    /* Go to sleep. */
    enqueue_send(dest, current_tcb);
    current_tcb->ipc_waiting_for = dest;
    return deactivate_self_schedule(
            is_call ? THREAD_STATE_WAIT_IPC_CALL : THREAD_STATE_WAIT_IPC_SEND);
}

/*
 * Send an IPC message to the destination thread.
 *
 * By the time we reach this functions, our registers containing the message
 * payload will have already been saved to our context frame. We are
 * responsible for just copying from there.
 */
NORETURN void
sys_ipc_send_c(word_t dest_thread, word_t operation)
{
    /* Ensure the thread exists. */
    if (EXPECT_FALSE(dest_thread >= max_tcbs)) {
        syscall_return_error(IPC_ERROR, EINVAL);
    }

    /* Find the destination thread. */
    tcb_t *dest = &tcbs[dest_thread];

    /* Take IPC lock. */
    spin_lock(&ipc_lock);

    /* Ensure the thread is still alive. */
    if (EXPECT_FALSE(!is_thread_alive(dest))) {
        spin_unlock(&ipc_lock);
        syscall_return_error(IPC_ERROR, EINVAL);
    }

    /* Do the IPC. */
    tcb_t *next = do_send(dest, OPERATION_IS_CALL(operation),
            OPERATION_IS_NON_BLOCKING(operation));

    /* Done. */
    spin_unlock(&ipc_lock);
    switch_to(next);
}

/* Send an IPC to another thread. */
NORETURN void
sys_ipc_recv(word_t src_thread, word_t operation)
{
    /* Ensure the thread exists. */
    if (EXPECT_FALSE(src_thread >= max_tcbs && src_thread != IPC_WAIT_ANY)) {
        syscall_return_error(IPC_ERROR, EINVAL);
    }

    spin_lock(&ipc_lock);

    /* Get first thread on send queue. */
    tcb_t *src = current_tcb->ipc_send_head;

    /* If there are no threads, go to sleep. */
    if (src == NULL) {
        current_tcb->ipc_waiting_for = src;
        tcb_t *next = deactivate_self_schedule(THREAD_STATE_WAIT_IPC_RECV);
        spin_unlock(&ipc_lock);
        switch_to(next);
    }

    /* Otherwise, dequeue the thread. */
    dequeue_send(current_tcb, src);

    /* Copy the message across and wake up the sender. */
    copy_message(src, current_tcb);

    /* Is the sender performing a call? */
    if (src->thread_state == THREAD_STATE_WAIT_IPC_CALL) {
        /* Move it into receive phase. 'ipc_waiting_for' should already be set
         * to us. */
        word_t tid = src->tid;
        ASSERT(src->ipc_waiting_for == current_tcb);
        src->thread_state = THREAD_STATE_WAIT_IPC_RECV;
        spin_unlock(&ipc_lock);
        syscall_return_success(tid);
        /* NOTREACHED */
    }

    /* If the sender was not performing a call, wake them up. */
    src->ipc_waiting_for = NULL;
    set_syscall_return_val_success(src, 0);
    set_syscall_return_val_success(current_tcb, src->tid);
    tcb_t *next = activate_schedule(src);
    spin_unlock(&ipc_lock);
    switch_to(next);
    /* NOTREACHED */
}

/*
 * Callbacks called on thread creation/deletion.
 */

void
sys_ipc_pre_thread_create_callback(tcb_t *tcb)
{
    spin_lock(&ipc_lock);
}

void
sys_ipc_post_thread_create_callback(tcb_t *tcb)
{
    spin_unlock(&ipc_lock);
}

void
sys_ipc_pre_thread_delete_callback(tcb_t *tcb)
{
    spin_lock(&ipc_lock);
}

void
sys_ipc_post_thread_delete_callback(tcb_t *tcb)
{
    spin_unlock(&ipc_lock);
}

