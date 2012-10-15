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


#include "macro.s"
#include <nano.h>

#include <arch/armv5.h>

INIT

noreturn_function __kernel_main

    /*
     * Kernel entry point for ARM.
     *
     * We enter here right after the bootloader has loaded our image
     * into physical memory. Our PC is currently a physical memory
     * address. We are responsible for getting the processor into
     * a sane state, enabling virtual memory and, after everything
     * is setup, jumping to the scheduler to start running our first
     * thread.
     */

    /* Move processor into supervisor state, interrupts disabled. */
    msr     cpsr_fc, #(SPSR_MODE_SUPERVISOR | SPSR_INTERRUPT_DISABLE| SPSR_FIQ_DISABLE)

    /* Setup kernel page table. */
    ldr     r0, kpagetable_phys
    mcr     p15, 0, r0, CP15_REG_TRANS_TABLE_BASE, c0, 0

    /* Setup userspace to have client-access to domain 0. */
    mov     r0, #1
    mcr     p15, 0, r0, CP15_REG_DOMAIN_ACCESS_CTRL, c0, 0

    /* Kill caches */
    mov     r0, #0
    mcr     p15, 0, r0, CP15_REG_CACHE_OPS, c7, 0

    /* Kill TLB */
    mov     r0, #0
    mcr     p15, 0, r0, CP15_REG_TLB_OPS, c7, 0

    /* Load first virtual memory code address. */
    ldr     r1, =init_vaddr

    /* Enable MMU / DCache / ICache / Vector relocation */
    ldr     r0, =CP15_CNTL_DEFAULT \
                        | CP15_CNTL_DCACHE_ENABLE \
                        | CP15_CNTL_ICACHE_ENABLE \
                        | CP15_CNTL_MMU_ENABLE \
                        | CP15_CNTL_SYSTEM_PROTECTION \
                        | CP15_CNTL_VECTOR_RELOCATE \
                        | CP15_CNTL_BRANCH_BUFF_ENABLE
    mcr     p15, 0, r0, CP15_REG_CONTROL, c0, 0

    /* Jump to our new address in virtual memory. */
    bx      r1

init_vaddr:
    /* Kill TLB */
    mov     r0, #0
    mcr     p15, 0, r0, CP15_REG_TLB_OPS, c7, 0

    /* Setup stack. */
    ldr     sp, =kernel_stack

    /* Perform a schedule. */
    bl      schedule
    bl      switch_to

end_function __kernel_main

/* Read only variables filled in by Elfweaver */
kpagetable_phys:
    .word 0

/* Kernel stack area. */
.section .stack, "aw"
.rept 1024
.byte 0
.endr
.global kernel_stack
kernel_stack:

DATA

/*
 * Symbols
 */
.global VADDR_BASE
.set VADDR_BASE, 0xffff0000
.global idle_tcb
.set idle_tcb, 0

