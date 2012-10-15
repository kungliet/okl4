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
 * Thread Control Block Definition.
 */

#ifndef __THREAD_H__
#define __THREAD_H__

#include <nano.h>

/* Thread states. */                        /* Owning subsystem */
#define THREAD_STATE_RUNNABLE       ( 0)    /* scheduler */
#define THREAD_STATE_WAIT_FUTEX     ( 2)    /* futex */
#define THREAD_STATE_WAIT_INT       ( 3)    /* interrupt */
#define THREAD_STATE_WAIT_SIGNAL    ( 4)    /* signals */
#define THREAD_STATE_WAIT_IPC_RECV  ( 5)    /* ipc */
#define THREAD_STATE_WAIT_IPC_SEND  ( 6)    /* ipc */
#define THREAD_STATE_WAIT_IPC_CALL  ( 7)    /* ipc */
#define THREAD_STATE_WAIT_JOIN      ( 8)    /* thread */
#define THREAD_STATE_HALTED         (-1)    /* thread */
#define THREAD_STATE_ZOMBIE         (-2)    /* thread */

#if !defined(ASSEMBLY)

/* Assembly functions. */
NORETURN void switch_to(tcb_t *next);

/* Maximum number of TCBs in the system. */
extern const unsigned int max_tcbs;

/* Address of the first TCB in the system. */
extern tcb_t * const tcbs;

INLINE static int
is_thread_alive(tcb_t * tcb)
{
    /* all alive states are >= 0 */
    /* tcb->thread_state is unsigned, so we need to check for this slightly differently */
    return tcb->thread_state < THREAD_STATE_ZOMBIE;
}

/*
 * Set the syscall return value for another thread.
 */
INLINE static void
set_syscall_return_val_success(tcb_t *tcb, int value)
{
    tcb->r0 = value;
}

/*
 * Set the syscall return value for another thread.
 *
 * The result and the error code are typically different. The result needs to
 * be a value that the syscall returns to the user to indicate that an error
 * occurred. The error code informs the user what _type_ of error occurred. At
 * this current point in time, the error code information can not be recovered
 * from userspace, nor is it tracked in the kernel.
 */
INLINE static void
set_syscall_return_val_error(tcb_t *tcb, int value, int error_code)
{
    tcb->r0 = value;
}

/*
 * Return from a syscall, indicating that the syscall was a success.
 */
INLINE NORETURN static void
syscall_return_success(int result)
{
    set_syscall_return_val_success(current_tcb, result);
    switch_to(current_tcb);
}

/*
 * Return from a syscall with an error code.
 */
INLINE NORETURN static void
syscall_return_error(int result, int error_code)
{
    set_syscall_return_val_error(current_tcb, result, error_code);
    switch_to(current_tcb);
}

#endif /* !ASSEMBLY */

#endif /* __THREAD_H__ */
