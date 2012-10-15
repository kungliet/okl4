/*
 * Copyright (c) 2006, National ICT Australia
 */
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

#include <compat/c.h>
#include <stdint.h>
#include <stdlib.h>
#include <threadstate.h>
#include <okl4/env.h>
#include <okl4/init.h>
#include <okl4/types.h>

extern void __thread_state_init(void *);

void __malloc_init(uintptr_t head_base,  uintptr_t heap_end);
void __sys_entry(void * env);
void __sys_thread_entry(void);
int main(int argc, char **argv);

#ifdef THREAD_SAFE
static struct thread_state __ts;
#endif

#if defined(OKL4_KERNEL_MICRO)
void *_okl4_forced_symbols = &okl4_forced_symbols;
#endif /* OKL4_KERNEL_MICRO */
void
__sys_entry(void *env)
{
    okl4_env_segment_t *heap;
    okl4_env_args_t *args;
    int argc = 0;
    char **argv = NULL;
    int result;

#if defined(OKL4_KERNEL_MICRO)
    /* Ensure forced symbols are linked into final binary.
     *
     * GCC is too smart for its own good, and compiles away any trivial
     * reference to 'forced_symbols'. Thus, we need to perform this next
     * bit of trickiness to confuse the compiler sufficiently to emit
     * the symbol.
     */
    if ((int)&_okl4_forced_symbols == 1) {
        for (;;);
    }
#endif /* OKL4_KERNEL_MICRO */
    /* Setup environment. */
    __okl4_environ = env;

    /* Get the heap address and size from the environment. */
    heap = okl4_env_get_segment("MAIN_HEAP");
    assert(heap != NULL);

    /* Get the command line arguments from the environment. */
    args = OKL4_ENV_GET_ARGS("MAIN_ARGS");
    if (args != NULL) {
        argc = args->argc;
        argv = &args->argv;
    }

    /* Initialise heap. */
    __malloc_init(heap->virt_addr, heap->virt_addr + heap->size);

#ifdef THREAD_SAFE
    __thread_state_init(&__ts);
#endif

    result = main(argc, argv);
    exit(result);
}

void
__sys_thread_entry(void)
{
    int result;

#ifdef THREAD_SAFE
    __thread_state_init(malloc(sizeof(struct thread_state)));
#endif
    result = main(0, NULL);
    exit(result);
}

/*
 * Libokl4 requires that a certain set of symbols in the library are
 * available at weave time. Unfortunately, the linker will discard
 * these symbols unless they are actively used by the binary we are
 * being linked against.
 *
 * This function will create a reference to those symbols, forcing
 * the linker to put them into the final binary, and hence making
 * them available to ElfWeaver.
 *
 * Once more, because we are ourself a library, these forced symbols must be
 * somewhere where they will always be linked against the user's binary. We
 * assume that the user will always call one of these init functions, pulling
 * these symbols in with it.
 */
#define FORCED_SYMBOL(x) \
    extern void *x; \
    static void **__forced_symbol__##x UNUSED = &x;
#if defined(OKL4_KERNEL_MICRO)
FORCED_SYMBOL(_okl4_utcb_memsec_lookup)
FORCED_SYMBOL(_okl4_utcb_memsec_map)
FORCED_SYMBOL(_okl4_utcb_memsec_destroy)
#endif /* OKL4_KERNEL_MICRO */

