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
/**
 *  * @file
 *   * @brief The ARM-specific bits of working with continuation functions.
 *    */
#ifndef __ARM__CONTINUATION_H__
#define __ARM__CONTINUATION_H__

#include <kernel/arch/config.h>
/* Define Stack and Continuation handling functions */

#if !defined(ASSEMBLY)
#include <kernel/arch/asm.h>
#if defined (__GNUC__)
#define ACTIVATE_CONTINUATION(continuation)     \
    do {                \
        __asm__ __volatile__ (                                          \
                "       orr     sp,     sp,     %1              \n"     \
                "       mov     pc,     %0                      \n"     \
                :                                                       \
                : "r" ((word_t)(continuation)),                         \
                  "i" (STACK_TOP)                                       \
                : "sp", "memory"                                        \
                );                                                      \
        while(1);                                                       \
    } while(false)


/* call a function with 2 arguments + a continuation so that the continuation can be retrieved by ASM_CONTINUATION */
NORETURN INLINE void call_with_asm_continuation(word_t argument0, word_t argument1, word_t continuation, word_t function)
{
    register word_t arg0    ASM_REG("r0") = argument0;
    register word_t arg1    ASM_REG("r1") = argument1;
    register word_t lr      ASM_REG("r14") = continuation;

    __asm__ __volatile__ (
        CHECK_ARG("r0", "%0")
        CHECK_ARG("r1", "%1")
        CHECK_ARG("lr", "%2")
        "       orr     sp,     sp,     %3              \n"
        "       mov     pc,     %4                      \n"
        :: "r" (arg0), "r" (arg1), "r" (lr), "i" (STACK_TOP),
        "r" (function)
    );
    while (1);
}

/* call a NORETURN function with 3 arguments + reset stack */
NORETURN INLINE void activate_function(word_t argument0, word_t argument1, word_t argument2, word_t function)
{
    register word_t arg0    ASM_REG("r0") = argument0;
    register word_t arg1    ASM_REG("r1") = argument1;
    register word_t arg2    ASM_REG("r2") = argument2;

    __asm__ __volatile__ (
        CHECK_ARG("r0", "%0")
        CHECK_ARG("r1", "%1")
        CHECK_ARG("r2", "%2")
        "       orr     sp,     sp,     %3              \n"
        "       mov     pc,     %4                      \n"
        :: "r" (arg0), "r" (arg1), "r" (arg2), "i" (STACK_TOP),
           "r" (function)
    );
    while (1);
}

#elif defined (__RVCT_GNU__)

C_LINKAGE NORETURN void jump_and_set_sp(word_t ret, addr_t context);
C_LINKAGE NORETURN void call_with_asm_continuation(word_t argument0, word_t argument1, word_t continuation, word_t function);
C_LINKAGE NORETURN void activate_function(word_t argument0, word_t argument1, word_t argument2, word_t function);

#define TOP_STACK       ((addr_t)(__current_sp() | STACK_TOP))

#define ACTIVATE_CONTINUATION(continuation)     \
    jump_and_set_sp((word_t)(continuation), (addr_t)TOP_STACK)


#elif defined(_lint)
void jump_and_set_sp(word_t ret, addr_t context);
void call_with_asm_continuation(word_t argument0, word_t argument1, word_t continuation, word_t function);
void activate_function(word_t argument0, word_t argument1, word_t argument2, word_t function);
#define TOP_STACK       ((tcb_t **)0)
#define ACTIVATE_CONTINUATION(continuation) ((continuation_t) continuation)()
#if defined(__cplusplus)
class tcb_t;
#else
typedef struct tcb tcb_t;
#endif
INLINE tcb_t * get_current_tcb(void)
{
    return NULL;
}
#else
#error "Unknown compiler"
#endif

#endif /* ! ASSEMBLY */

#endif /* ! __ARM__CONTINUATION_H__ */
