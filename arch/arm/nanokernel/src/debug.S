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
 * Debugging system calls.
 */

#include "macro.s"
#include <nano.h>
#include <tcb_offsets.h>
#include <arch/armv5.h>
#include <arch/armv5_instructions.h>

#if defined(CONFIG_PERF)

DATA

lvariable perf_counters_enabled, .word 0

CODE

/*
 * debug_kill_cache
 *
 * Clear icache/dcache.
 */

noreturn_function debug_kill_cache
    adr     lr, debug_return
    b       plat_clean_invalidate_cache

end_function debug_kill_cache

/*
 * debug_cycles
 *
 * Read a cycle counter.
 */

noreturn_function debug_get_cycles

    /* Determine if we have already turned on the performance counters. */
    LDR_OFF(COND_AL, REG_R0, perf_counters_enabled)
    cmp     r0, #1
    bne     try_enable_performance_counters

get_counters:
    adr     lr, debug_return
    b       plat_read_performance_counter

try_enable_performance_counters:
    /* Have we already decided that the performance counter is broken? */
    cmp     r0, #-1
    beq     debug_return

    bl      plat_enable_performance_counters
    /* Remember for future calls that the counter has been turned on, or
     * that it is broken. */
    STR_OFF(COND_AL, REG_R0, perf_counters_enabled)

    b       get_counters

end_function debug_get_cycles

/*
 * Return to user from a debugging syscall.
 */
function debug_return

    /* Load current TCB. */
    ldr     r5, =current_tcb
    ldr     r13, [r5]

    /* Restore user registers, starting from r4. */
    add     r6, r13, #16
    ldmia   r6, {r4 - r14}^
    nop

    /* Load PC, and return back to it. */
    ldr     r14, [r13, #TCB_PC]
    subs    pc, r14, #0

end_function debug_return

#endif

