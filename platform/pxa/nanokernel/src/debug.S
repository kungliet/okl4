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
 * Platform Performance Operations
 */

#include "macro.s"
#include <nano.h>
#include <tcb_offsets.h>
#include <arch/armv5.h>

#if defined(CONFIG_PERF)

#define DCACHE_SIZE          (32 * 1024)
#define DCACHE_LINE_SIZE     (32)
#define DCACHE_SETS          (32)
#define DCACHE_LINES         (1024)

CODE


function plat_clean_invalidate_cache

    /* Drain write buffer. */
    mov     r0, #0
    mcr     p15, 0, r0, CP15_REG_CACHE_OPS, c10, 4

    /* Clean data cache. */
    mov     r1, #0
    mov     r0, #(DCACHE_LINES / 2)
1:
    mcr     p15, 0, r1, CP15_REG_CACHE_OPS, c2, 5
    add     r1, r1, #DCACHE_LINE_SIZE
    mcr     p15, 0, r1, CP15_REG_CACHE_OPS, c2, 5
    add     r1, r1, #DCACHE_LINE_SIZE
    subs    r0, r0, #1
    bne     1b

    /* Invalidate the entire cache. */
    mov     r0, #0
    mcr     p15, 0, r0, CP15_REG_CACHE_OPS, c7, 0

    /* Flush branch target cache. */
    mov     r0, #0
    mcr     p15, 0, r0, CP15_REG_CACHE_OPS, c5, 6

    mov     pc, lr
end_function plat_clean_invalidate_cache


#define READ_CYCLES 1

function plat_read_performance_counter
    mov     r1, #0
#if READ_CYCLES
    mrc     p14, 0, r0, c1, c1, 0
#else
    mrc     p14, 0, r0, c2, c0, 0
#endif
    mov     pc, lr
end_function plat_read_performance_counter


#define EVENT_DATA_DEP_STALL            0x02
#define EVENT_BRANCH_MISPREDICT         0x06
#define EVENT_INSTRUCTIONS_EXECUTED     0x07

#define PMNC_EVENT_0(x)     ((x) << 12)
#define PMNC_EVENT_1(x)     ((x) << 20)
#define PMNC_DIVIDE_64      (1 << 3)
#define PMNC_CLOCK_RESET    (1 << 2)
#define PMNC_COUNTER_RESET  (1 << 1)
#define PMNC_ENABLE         (1 << 0)

function plat_enable_performance_counters
    /* Turn on the performance counter. */
    ldr     r0, =(PMNC_ENABLE | PMNC_CLOCK_RESET | PMNC_COUNTER_RESET | PMNC_EVENT_0(EVENT_INSTRUCTIONS_EXECUTED))
    mcr     p14, 0, r0, c0, c1, 0

    /* Make sure it works. If it doesn't, we just return (-1) from now on. This
     * works around a skyeye annoyance which causes it to spew error messages
     * to screen each time the counter is accessed. */
    mrc     p14, 0, r0, c1, c1, 0
    mrc     p14, 0, r1, c1, c1, 0
    cmp     r0, r1
    movne   r0, #1   /* Counter changed. Assume it works. */
    moveq   r0, #-1  /* Counter didn't change. Assume it is broken. */

    mov     pc, lr
end_function plat_enable_performance_counters

#endif
