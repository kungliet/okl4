/*
 * Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
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

#include <rt_misc.h>
#include <stdio.h>
#include <assert.h>
#include <okl4/env.h>
#include <okl4/types.h>
#include <okl4/init.h>

#pragma import(__use_two_region_memory)

__value_in_regs struct __initial_stackheap
__user_initial_stackheap(unsigned r0, unsigned sp, unsigned r2, unsigned sl)
{
    struct __initial_stackheap config;
    okl4_word_t env_base;
    okl4_env_segment_t *heap;
    okl4_env_segment_t *stack;

#if defined(OKL4_KERNEL_MICRO)
    /*
     * Ensure forced symbols are linked into final binary.
     *
     * RVCT is too smart for its own good, and compiles away any trivial
     * reference to 'forced_symbols'. Thus, we need to perform this next
     * bit of trickiness to confuse the compiler sufficiently to emit
     * the symbol.
     */
    if ((int)&okl4_forced_symbols == 1) {
        for (;;);
    }
#endif /* OKL4_KERNEL_MICRO */

    /* Get the environment from the tcb. */
    L4_StoreMR(1, &env_base);
    __okl4_environ = (okl4_env_t *)env_base;

    /* Get the heap and stack from the environment. */
    heap = okl4_env_get_segment("MAIN_HEAP");
    assert(heap != NULL);

    stack = okl4_env_get_segment("MAIN_MAIN_STACK");
    assert(stack != NULL);

    config.heap_base   = (unsigned)heap->virt_addr;
    config.stack_base  = (unsigned)(stack->virt_addr + stack->size);
    assert(config.stack_base % 8 == 0);
    config.heap_limit  = (unsigned)(heap->virt_addr + heap->size);
    config.stack_limit = (unsigned)stack->virt_addr;

    return config;
}

unsigned
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION < 200000)
__user_heap_extend(int ignore, unsigned requestedsize, void **base)
#else
__user_heap_extend(int ignore, void **base, unsigned requestedsize)
#endif
{
    printf("********************** %s: ERROR ******************\n", __func__);
    return 0;
}
