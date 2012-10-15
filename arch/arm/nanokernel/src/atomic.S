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
 * ARMv5 User Helpers
 *
 * This file contains code run in usermode that the kernel provides to the user
 * to help them carry out operations that they could not perfrom themselves,
 * but would be too expensive to perform syscalls for, such as atomic
 * operations.
 *
 * We implement psuedo atomic operations by having some code in this page
 * perform a standard sequence of instructions, which may be interruptted at an
 * arbitray time by an interrupt or fault.
 *
 * If either of these operations occur, the thread interrupted in its
 * compare-and-set will eventually been switch_to()'ed back to. In switch_to(),
 * we determine if the interruptted vaddr was in the range 0xffff0000 to
 * 0xffffffff and, if so, perform a fixup of the operation.
 *
 * FIXUP OPERATION:
 *
 *     At the moment, the fixup operation is very simple. We divide up
 *     instructions into blocks of four. If we are preempted in the
 *     middle of a block, we restart to that block.
 *
 *     We will probably need something a little more sophisticated in
 *     the future.
 */

#include "macro.s"
#include <nano.h>

.section .user_helpers, "ax"

/*
 * 0xfffef000 : Atomic Compare and Set:
 *
 *     void okn_atomic_cmp_set(uint32_t *addr, uint32_t test, uint32_t set);
 *
 * Test the value at (*addr) to see if it is equal to 'test'. If so, set
 * it to a new value and return true. Otherwise, return false.
 *
 * We scratch r1 - r3.
 *
 * This function is called by the user and runs in usermode.
 */
.balign 32
function user_atomic_cmp_set
    ldr     r3,     [r0]    /* RESTART POINT */
    nop
    cmp     r1,     r3
    streq   r2,     [r0]
    moveq   r0,     #1      /* RESTART POINT */
    movne   r0,     #0
    bx      lr
end_function user_atomic_cmp_set

/*
 * 0xfffef020 : Atomic Add:
 *
 *     uint32_t okn_atomic_add(uint32_t *addr, int32_t val);
 *
 * Increment the value at the given address by the given amount,
 * and return the new value.
 *
 * We scratch r0 - r2.
 *
 * This function is called by the user and runs in usermode.
 */
.balign 32
function user_atomic_add
    ldr     r2, [r0]       /* RESTART POINT */
    nop
    add     r2, r2, r1
    str     r2, [r0]
    mov     r0, r2         /* RESTART POINT */
    bx      lr
end_function user_atomic_add

CODE

/*
 * We come here if the user was preempted in the middle of a user helper
 * function. We need to work out what the user was doing, and what address
 * we should return them to.
 *
 * At this stage, all user registers are loaded. Our banked 'lr' contains
 * the user's return address, and our banked 'sp' is available for scratch.
 * Any register the user is happy to restore on a restart we may also clobber.
 *
 * We are responsbile for returning to the user ourself.
 */
function user_helper_fixup
    /* Jump back to the first of the last block of four instructions. */
    /*
     * We cannot use ands pc, lr, 0xfffffff0 here because the assembler
     * translates it to a bics instruction which cannot return to user mode
     */
    /*
     * We use movs here instead of ands because QEMU does not support ands
     * to pc register with mode change
     */
    mov     sp,     #0xfffffff0
    and     sp,     lr,     sp
    movs    pc,     sp
end_function user_helper_fixup

