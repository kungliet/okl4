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
 * Optimised scheduler paths.
 */

#include "macro.s"
#include <nano.h>
#include <tcb_offsets.h>
#include <arch/armv5_instructions.h>
#include <thread.h>

CODE

/*
 * Enqueue a thread onto the scheduler queue.
 *
 * Inputs:
 *      r0  - Thread to enqueue.
 *
 * Clobbers:
 *      r0 - r6
 */
#if ASM_ENQUEUE
function enqueue

    /* Get current thread's priority. */
    ldr     r3, [r0, #TCB_PRIORITY]

    /* Load priority bitmap. */
    LDR_OFF(COND_AL, REG_R2, priority_bitmap)

    /* Load priority head. */
    LDR_OFF(COND_AL, REG_R1, priority_heads)
    ldr     r6, [r1, r3, lsl #2]

    /* Save priority bitmap back to memory. */
    mov     r4, #1
    orr     r4, r2, r4, lsl r3
    STR_OFF(COND_AL, REG_R4, priority_bitmap)

    /* Is the head pointer NULL? */
    cmp     r6, #0

    /* Only a single thread. */
    streq r0, [r1, r3, lsl #2]
    streq   r0, [r0, #TCB_NEXT]
    streq   r0, [r0, #TCB_PREV]
    bxeq    lr

1:
    /* Otherwise, load up next and previous pointers. */
    ldr     r4, [r6, #TCB_PREV]
    str     r0, [r6, #TCB_PREV]
    str     r6, [r0, #TCB_NEXT]
    str     r0, [r4, #TCB_NEXT]
    str     r4, [r0, #TCB_PREV]
    bx      lr

end_function enqueue
#endif /* ASM_ENQUEUE */

/*
 * Find a runnable thread on the scheduler queue and dequeue it.
 *
 * Clobbers:
 *      r0 - r6
 *
 * Outputs:
 *      r0  - Thread to run.
 */
#if ASM_DO_SCHEDULE
function do_schedule

    /* Load priority bitmap. */
    LDR_OFF(COND_AL, REG_R2, priority_bitmap)
    LDR_OFF(COND_AL, REG_R4, priority_heads)

    /* Determine highest priority thread. If idle, r3 will become (-1). */
    clz     r3, r2
    rsb     r3, r3, #31

    /* Load that thread. If idle, we reload priority bitmap, which is
     * located just before the first priority head. (It doesn't matter
     * what we load, as long as we can load _something_.) */
    ldr     r0, [r4, r3, lsl #2]

    /* Is the thread idle? */
    cmp     r2, #0
    beq     1f

    /* Need to clear the bit in the bitmap if no other threads are on
     * the queue. Start by loading in a bit to shift. */
    mov     r6, #1

    /* Load next pointer. */
    ldr     r5, [r0, #TCB_NEXT]

    /* Clear the appropriate bit in the bitmap. */
    bic     r2, r2, r6, lsl r3

    /* Load the previous pointer. */
    ldr     r6, [r0, #TCB_PREV]

    /* Dequeue the thread. If we are the only thread in the system,
     * we merely write NULL to the head pointer. Otherwise, we need
     * to update the next and previous TCBs pointers. */
    cmp     r5, r0
    moveq   r5, #0
    str     r5, [r4, r3, lsl #2]
    strne   r6, [r5, #TCB_PREV]
    strne   r5, [r6, #TCB_NEXT]

    /* Clear next/previous pointers. */
    mov     r3, #0
    str     r3, [r0, #TCB_NEXT]
    str     r3, [r0, #TCB_PREV]

    /* Update bitmap if necessary. */
    STR_OFF(COND_EQ, REG_R2, priority_bitmap)
    bx      lr

1:
    /* Return idle. */
    mov     r0, #0
    bx      lr

end_function do_schedule
#endif /* ASM_DO_SCHEDULE */

/*
 * Activate the given thread, and then perform a schedule.
 *
 * Clobbers:
 *      r0 - r7
 *
 * Outputs:
 *      r0  - Thread to run.
 */
#if ASM_ACTIVATE_SCHEDULE
function activate_schedule

    /* Load the current TCB. */
    LDR_OFF(COND_AL, REG_R1, current_tcb)

    /* Load priority of the destination thread. */
    ldr     r4, [r0, #TCB_PRIORITY]

    /* Activate the newly provided thread. */
    mov     r3, #THREAD_STATE_RUNNABLE

    /* Determine if the current thread is idle. */
    cmp     r1, #0

    /* Load priority of current thread. */
    ldrne   r5, [r1, #TCB_PRIORITY]

    /* Save new state of the newly provided thread. */
    str     r3, [r0, #TCB_THREAD_STATE]

    /* If the current thread is idle, we can always just run the new thread. */
    bxeq    lr

    /* Compare the priorities of the two threads. */
    cmp     r4, r5
    bgt     1f

    /* The currently running thread has the higher priority. Enqueue the
     * new thread, and return the currently running thread. */
    str     r1, [sp, #-4]!
    mov     r7, lr
    bl      enqueue /* Does not clobber r7. */
    ldr     r0, [sp], #4
    bx      r7

1:
    /* The new thread has a higher priority than the current running thread.
     * Enqueue the currently running thread. */
    str     r0, [sp, #-4]!
    mov     r7, lr
    mov     r0, r1
    bl      enqueue /* Does not clobber r7. */
    ldr     r0, [sp], #4
    bx      r7

end_function activate_schedule
#endif /* ASM_ACTIVATE_SCHEDULE */

