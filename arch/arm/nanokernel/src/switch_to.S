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
#include <tcb_offsets.h>
#include "macro.s"

#include <arch/armv5.h>
#include <arch/armv5_instructions.h>

#if !defined(CONFIG_USE_TANGLED_FASTPATH)

CODE

/*
 * NORETURN void switch_to(tcb_t *destination)
 *
 * Inputs:
 *     r0  - Destination thread to switch to.
 *
 * Outputs:
 *     Function does not return.
 */
.global test

noreturn_function switch_to

    /* Install tcb into 'current_tcb' memory. */
    STR_OFF(COND_AL, REG_R0, current_tcb)   /* 0020 */

    /* Is the destination thread idle thread? */
    cmp     r0, #0                                                   /* 0030 */
    beq     idle_thread                                              /* 0040 */

#if TCB_OFS_R0 != 0
#error "switch_to() requires r0-r15 to be at the beginning of the TCB."
#endif

    /* Restore user CPSR. */
    ldrne   r1, [r0, #TCB_CPSR]                                      /* 0070 */
    msr     spsr, r1                                                 /* 0080 */

    /* Load the user's PC. */
    ldr     r14, [r0, #TCB_PC]                                       /* 0085 */

    /* Restore user registers. */
    ldmia   r0, {r0 - r14}^                                          /* 0090 */

    /* Can't use banked registers r13/r14 the cycle after a ldmia^. */
    nop                                                              /* 0095 */

    /* If the user was preempted in a user-helper function (code located in the
     * range 0xff000000 -- 0xffffffff), perform a fixup. To test for this, we
     * just see if they were preempted in the range 0xf0000000 - 0xffffffff,
     * which should normally just be kernel code or the helpers. */
    /* Note, this check defines the start of kernel address space, actual data 
     * starts at 0xfffef000, but due to this check 0xff000000 and up is reserved */
    cmp     r14, #0xff000000                                         /* 0110 */
    bhi     user_helper_fixup                                        /* 0120 */

    /* Return to user. */
    sublos  pc, r14, #0                                              /* 0130 */

    /* Otherwise, do the fixup. */

end_function switch_to


#endif /* !defined(CONFIG_USE_TANGLED_FASTPATH) */

