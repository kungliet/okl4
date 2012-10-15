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
 * Description:   Kernel Resource Manager
 */
#ifndef __KMEM_RESOURCE_H__
#define __KMEM_RESOURCE_H__

#include <kmemory.h>
#include <smallalloc.h>
#include <kernel/debug.h>

extern word_t pre_tcb_init;
class space_t;

/*
 * All small_alloc pool and kmem pool should be registered here,
 * note that small_alloc pool always has smaller numbers, that
 * is because they are used as index to small_alloc_pools.
 * When adding or deleting new entry, don't forget to add/delete
 * entry in __kmem_group_names[] as well.
 */
typedef enum {
    /* start of small_alloc pools */
    kmem_group_mutex,
    kmem_group_space,
    kmem_group_tcb,
#if defined(ARCH_ARM) && (ARCH_VER == 5)
    kmem_group_l0_allocator,
    kmem_group_l1_allocator,
#endif
    /* end of small_alloc pools, below are kmem pools */
    kmem_group_clist,
    kmem_group_clistids,
    kmem_group_root_clist,
    kmem_group_ll,
    kmem_group_misc,
    kmem_group_mutexids,
    kmem_group_pgtab,
    kmem_group_resource,
    kmem_group_spaceids,
    kmem_group_stack,
    kmem_group_trace,
    kmem_group_utcb,
    kmem_group_irq,
    kmem_group_physseg_list,
    max_kmem_group
} kmem_group_e;

#define MAX_KMEM_GROUP  (word_t) max_kmem_group
#define MAX_KMEM_SMALL_ALLOC_GROUP kmem_group_clist

class kmem_resource_t
{
private:
    kmem_t          heap;  /* per resource pool heap */
    small_alloc_t   small_alloc_pools[MAX_KMEM_SMALL_ALLOC_GROUP];
#if defined(CONFIG_KMEM_TRACE)
    macro_set_t     macro_set_kmem_groups;
    macro_set_entry_t macro_set_entry_kmem_groups[MAX_KMEM_GROUP];
    macro_set_entry_t * macro_set_list_kmem_groups[MAX_KMEM_GROUP];
    word_t          group_number;
#endif
    kmem_group_t    kmem_groups[MAX_KMEM_GROUP];
    spinlock_t      lock;

    /* Init functions that may later be removed if this object
     * is initialized by elf-weaver.
     */

    void init_heap(void * start, void *end)
    {
        heap.init(start, end);
    }

    void init_kmem_groups(void)
    {
        word_t i;
        for (i = 0; i < MAX_KMEM_GROUP; i++)
        {
#if defined(CONFIG_KMEM_TRACE)
            kmem_groups[i].mem = 0;
            kmem_groups[i].name = __kmem_group_names[i];
            macro_set_entry_kmem_groups[i].set = &macro_set_kmem_groups;
            macro_set_entry_kmem_groups[i].entry = (addr_t) &kmem_groups[i];
            macro_set_list_kmem_groups[i] = &macro_set_entry_kmem_groups[i];
#else
            kmem_groups[i] = 0;
#endif
        }
#if defined(CONFIG_KMEM_TRACE)
        macro_set_kmem_groups.list = macro_set_list_kmem_groups;
        group_number = MAX_KMEM_GROUP;
        macro_set_kmem_groups.entries = &group_number;
#endif
    }

    void arch_init_small_alloc_pools(void);

    void init_small_alloc_pools(void);

public:
    /* Disable copy constructor */
    kmem_resource_t(const kmem_resource_t&);
    /* Disable operator = */
    kmem_resource_t& operator= (const kmem_resource_t&);

    bool is_small_alloc_group(kmem_group_e group) {
        bool ret;
        ret = (group == kmem_group_space) || (group == kmem_group_tcb) ||
             (group == kmem_group_mutex);
#if defined(ARCH_ARM) && (ARCH_VER == 5)
        ret = ret || (group == kmem_group_l0_allocator) ||
             (group == kmem_group_l1_allocator);
#endif
        return ret;
    }

    void init(void *end);

    void * alloc(kmem_group_e group, bool zeroed);
    void * alloc(kmem_group_e group, word_t size, bool zeroed);
    void * alloc_aligned(kmem_group_e group, word_t size,
                         word_t alignment, word_t mask, bool zeroed);
    void free(kmem_group_e group, void * address, word_t size = 0);
    void add(void * address, word_t size) {heap.add(address, size);}

    friend class small_alloc_t;
    friend class kdb_t;

#if defined(CONFIG_KDB_CONS)
    kmem_resource_t *next;
#endif
};

extern kmem_resource_t * get_current_kmem_resource(void);

/* If you have a KDB console, allow selection of resource for reporting */
#if defined(CONFIG_KDB_CONS)
extern kmem_resource_t *__resources;
#endif

#endif /* !__KMEM_RESOURCE_H__ */
