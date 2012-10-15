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
 * Nano Test Arch-Specific Performance Functionality
 */

#ifndef PERFORMANCE_ARCH_H
#define PERFORMANCE_ARCH_H

#include <nano/nano.h>

/* Should we flush the ARM in-CPU branch target buffer prior to performance
 * testing? Doing so makes the results larger (because branch prediction is
 * effectively disabled), but stabilises the results for comparative testing,
 * which is useful when optimising at a cycle level. */
#define FLUSH_ARM_BRANCH_TARGET_BUFFER  1

#if !FLUSH_ARM_BRANCH_TARGET_BUFFER
/* Don't flush the buffer. */
#define invalidate_arm_branch_target_buffer() \
        while { } (0)
#endif

/* Test start. */
#define start_test(x)                                                         \
    do {                                                                      \
        invalidate_arm_branch_target_buffer();                                \
        __asm__ __volatile__ (                                                \
                "ldr     r4, =perf_start_cycle  \n\t"                         \
                "mov     r0, #0                 \n\t"                         \
                "str     r0, [r4]               \n\t"                         \
                "bl      okn_syscall_get_cycles \n\t"                         \
                "bl      okn_syscall_get_cycles \n\t"                         \
                "nop; nop; nop; nop             \n\t"                         \
                ".balign 32                     \n\t"                         \
                "bl      okn_syscall_get_cycles \n\t"                         \
                "nop; nop                       \n\t"                         \
                "str     r0, [r4]               \n\t"                         \
                "nop; nop; nop; nop             \n\t"                         \
                : /* no inputs */                                             \
                : /* no outputs */                                            \
                : "r0", "r1", "r2", "r3",                                     \
                  "r4", "r12", "lr", "cc", "memory");                         \
        LABEL(x);                                                             \
    } while(0)

/* Test end. */
#define end_test(x)                                                           \
    do {                                                                      \
        LABEL(x);                                                             \
        __asm__ __volatile__ (                                                \
                "nop; nop; nop; nop             \n\t"                         \
                "bl      okn_syscall_get_cycles \n\t"                         \
                "ldr     r4, =perf_end_cycle    \n\t"                         \
                "str     r0, [r4]               \n\t"                         \
                : /* no inputs */                                             \
                : /* no outputs */                                            \
                : "r0", "r1", "r2", "r3",                                     \
                  "r4", "r12", "lr", "cc", "memory");                         \
        okn_syscall_ipc_send(0, 0, 0, 0, 0, 0, 0, 0, 0);                      \
    } while(0)

/* Flush out the ARM branch target buffer. */
void invalidate_arm_branch_target_buffer(void);

#endif /* PERFORMANCE_ARCH_H */

