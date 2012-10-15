/*
 * Copyright (c) 2002, 2003-2004, Karlsruhe University
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
/*
 * Description:  Kernel Resource Manager
 */

#include <l4.h>
#include <kmem_resource.h>
#include <space.h>
#include <tcb.h>
#include <kdb/tracepoints.h>

DECLARE_TRACEPOINT (KMEM_ALLOC);
DECLARE_TRACEPOINT (KMEM_FREE);

#if defined(CONFIG_KDB_CONS)
kmem_resource_t *__resources = NULL;
#endif

#if defined(CONFIG_KMEM_TRACE)
const char* __kmem_group_names[MAX_KMEM_GROUP] = {
    "kmem_mutex",
    "kmem_space",
    "kmem_tcb",
#if defined(ARCH_ARM) && (ARCH_VER == 5)
    "kmem_l0_allocator",
    "kmem_l1_allocator",
#endif
    "kmem_clist",
    "kmem_clistids",
    "kmem_root_clist",
    "kmem_ll",
    "kmem_misc",
    "kmem_mutexids",
    "kmem_pgtab",
    "kmem_resource",
    "kmem_spaceids",
    "kmem_stack",
    "kmem_trace",
    "kmem_utcb",
    "kmem_irq",
    "kmem_physseg_list"
};
#endif

kmem_resource_t * get_current_kmem_resource()
{
    kmem_resource_t *ret;
    space_t * current_space;

    /* if current_tcb is not setup, use kernel space. */
    if (pre_tcb_init)
    {
        current_space = get_kernel_space();
    }
    else
    {
        current_space = get_current_space();
    }
    if (current_space == NULL) current_space = get_kernel_space();

    ret = current_space->get_kmem_resource();
    /* We should've already checked that we have the privilege
     * (have the access to kmem_resource object)
     * to create any kernel object at the beginning of the
     * system call, so that we add assertion here to double
     * check instead of return NULL pointer to indicate failure.
     */

    ASSERT(ALWAYS, ret != NULL);
    return ret;
}

void kmem_resource_t::init_small_alloc_pools(void)
{
    /* init small alloc pool with no max limit */
    small_alloc_pools[kmem_group_space].init(&kmem_groups[kmem_group_space],
            sizeof(space_t));
    small_alloc_pools[kmem_group_tcb].init(&kmem_groups[kmem_group_tcb],
            KTCB_SIZE);
    small_alloc_pools[kmem_group_mutex].init(&kmem_groups[kmem_group_mutex],
            sizeof(mutex_t));
    arch_init_small_alloc_pools();
}

void kmem_resource_t::init(void *end)
{
#if defined(CONFIG_DEBUG)
    word_t size = (word_t)end - (word_t)this;
#if defined(CONFIG_KDB_CONS)
    this->next = NULL;
    if (__resources == NULL) {
        __resources = this;
    } else {
        kmem_resource_t *current = __resources;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = this;
    }
#endif
#endif
    ASSERT(ALWAYS, (size % KMEM_CHUNKSIZE) == 0);
    ASSERT(ALWAYS, sizeof(kmem_resource_t) <= KMEM_CHUNKSIZE);
    lock.init();
    init_heap((void *)((word_t)this + KMEM_CHUNKSIZE), end);
    init_kmem_groups();
    init_small_alloc_pools();
}

void * kmem_resource_t::alloc(kmem_group_e group, bool zeroed)
{
    void * ret;
    lock.lock();
    ASSERT(ALWAYS, is_small_alloc_group(group));
    ret = small_alloc_pools[group].allocate(zeroed);
    lock.unlock();
    TRACEPOINT_TB(KMEM_ALLOC,
                  printf("kmem_alloc_small(%x), ip: %p ==> %p\n",
                         (int)group, __return_address(), ret),
                  "kmem_alloc_small(%x), ip: %lx ==> %lx\n",
                  (int)group, (word_t)__return_address(), (word_t)ret);
    return ret;
}

void * kmem_resource_t::alloc(kmem_group_e group, word_t size, bool zeroed)
{
    void * ret;
    lock.lock();
    ASSERT(ALWAYS, !is_small_alloc_group(group));
    ret = heap.alloc(&kmem_groups[group], size, zeroed);
    lock.unlock();
    TRACEPOINT_TB(KMEM_ALLOC,
                  printf("kmem_alloc(%lx), ip: %p ==> %p\n",
                         size, __return_address(), ret),
                  "kmem_alloc(%lx), ip: %lx ==> %lx\n",
                  size, (word_t)__return_address(), (word_t)ret);
    return ret;
}

#if defined(CONFIG_ARCH_MIPS) || (defined(CONFIG_ARM_VER) && (CONFIG_ARM_VER == 6))
void * kmem_resource_t::alloc_aligned(
    kmem_group_e group, word_t size, word_t alignment, word_t mask, bool zeroed)
{
    void * ret;
    lock.lock();
    ASSERT(ALWAYS, !is_small_alloc_group(group));
    ret = heap.alloc_aligned(&kmem_groups[group], size, alignment, mask, zeroed);
    lock.unlock();
    TRACEPOINT_TB(KMEM_ALLOC,
                  printf("kmem_alloc_aligned(%lx), ip: %p ==> %p\n",
                         size, __return_address(), ret),
                  "kmem_alloc_aligned(%lx), ip: %lx ==> %lx\n",
                  size, (word_t)__return_address(), (word_t)ret);
    return ret;
}
#endif

void kmem_resource_t::free(kmem_group_e group, void * address, word_t size)
{
    lock.lock();
    if (size == 0)
    {
        ASSERT(ALWAYS, is_small_alloc_group(group));
        small_alloc_pools[group].free(address);
    }
    else
    {
        ASSERT(ALWAYS, !is_small_alloc_group(group));
        heap.free(&kmem_groups[group], address, size);
    }
    lock.unlock();
    TRACEPOINT_TB(KMEM_FREE,
                  printf ("kmem free (%p, %lx), ip: %p\n",
                          address, size,
                          __return_address()),
                  "kmem_free (%lx, %lx), ip=%lx\n",
                  (word_t)address, size, (word_t)__return_address());
}

