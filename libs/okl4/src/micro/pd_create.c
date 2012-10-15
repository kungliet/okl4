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

#include <assert.h>

#include <l4/config.h>

#include <okl4/types.h>
#include <okl4/pd.h>
#include <okl4/env.h>

#include "pd_helpers.h"

/*
 * Setup the layout of the struct.
 *
 * As the protection domain structure contains multiple variable-sized
 * data structures within it, we are required to perform some magic to
 * get it setup.
 *
 * This takes place by allocating more space for the okl4_pd_t object than is
 * strictly required for the struct. The extra "slack" space we can then carve
 * up into smaller regions that we can perform "allocations" from.
 */
static void
allocate_pd_slack_memory(okl4_pd_t *pd, okl4_pd_attr_t *attr)
{
    /* We flagrantly abuse memory in this function. */
    /*lint --e{826}*/

    char *location;

    /* Statically sized. */
    pd->kspace = &pd->_init.kspace_mem;

    /* The first object after the okl4_pd_t data is our kclist. */
    location = (char *)pd + sizeof(okl4_pd_t);
    pd->_init.kclist = (okl4_kclist_t *)location;
    location += OKL4_KCLIST_SIZE(attr->kclist_size);

    /* After the kclist comes our utcb_area. */
    pd->_init.utcb_area = (okl4_utcb_area_t *)location;
    location += OKL4_UTCB_AREA_SIZE(attr->max_threads);

    /* After the utcb_area we have kthread memory. */
    pd->thread_pool = (okl4_kthread_t *)location;
    location += _OKL4_PD_THREADS_POOL_SIZE(attr->max_threads);

    /* And after kthread memory there is a bitmap allocator to manage the above
     * memory */
    pd->thread_alloc = (okl4_bitmap_allocator_t *)location;
    location += OKL4_BITMAP_ALLOCATOR_SIZE(attr->max_threads);

    /* Ensure size was correct. */
    assert((unsigned int)(location - (char *)pd) == OKL4_PD_SIZE_ATTR(attr));
}

/*
 * Construct a new okl4_kclist_t using pools provided by the user.
 */
static int
construct_kclist(okl4_pd_t *pd, okl4_pd_attr_t *attr)
{
    okl4_kclist_attr_t kclist_attr;
    okl4_kclistid_t id;
    int error;

    assert(!(pd->_init.kclist == NULL && pd->_init.kclistid_pool == NULL));

    /* If the user provided us with a kclist, we need not make one. */
    if (attr->kclist != NULL) {
        pd->_init.kclist_id.raw = ~0UL;
        pd->_init.kclist = attr->kclist;
        return 0;
    }

    /* Allocate a kclist identifier. */
    error = okl4_kclistid_allocany(pd->_init.kclistid_pool, &id);
    if (error) {
        return error;
    }
    pd->_init.kclist_id = id;

    /* Create the kclist object. */
    okl4_kclist_attr_init(&kclist_attr);
    okl4_kclist_attr_setid(&kclist_attr, id);
    okl4_kclist_attr_setsize(&kclist_attr, attr->kclist_size);
    error = okl4_kclist_create(pd->_init.kclist, &kclist_attr);
    if (error) {
        okl4_kclistid_free(pd->_init.kclistid_pool, id);
        return error;
    }

    return 0;
}

/*
 * Destruct the okl4_kclist_t created by the pd.
 */
static void
destruct_kclist(okl4_pd_t *pd)
{
    assert(pd->_init.kclistid_pool != NULL);

    okl4_kclist_delete(pd->_init.kclist);
    okl4_kclistid_free(pd->_init.kclistid_pool, pd->_init.kclist_id);
}

/*
 * Construct a new okl4_utcb_area_t using pools provided by the user.
 */
static int
construct_utcb_area(okl4_pd_t *pd, okl4_pd_attr_t *attr)
{
    okl4_utcb_area_attr_t utcb_area_attr;
    okl4_word_t utcb_area_size;
    int error;

    assert(pd->_init.utcb_area != NULL);
    assert(pd->_init.virtmem_pool != NULL);

    /*
     * Allocate virtual memory for the UTCB area.
     *
     * Because we need to align the base of the UTCB area to its size, we have
     * to do some tricks to get an appropraite virtual memory area. It turns
     * out that if we allocate twice the required size, we can be sure we can
     * find a subregion which is correctly aligned.
     */
    utcb_area_size = ROUND_UP(attr->max_threads * UTCB_SIZE,
            OKL4_DEFAULT_PAGESIZE);
    okl4_range_item_setsize(&pd->_init.utcb_area_virt_item, utcb_area_size * 2);
    error = okl4_virtmem_pool_alloc(pd->_init.virtmem_pool,
            &pd->_init.utcb_area_virt_item);
    if (error) {
        return error;
    }

    /* Initialise the okl4_utcb_area_t. */
    okl4_utcb_area_attr_init(&utcb_area_attr);
    okl4_utcb_area_attr_setrange(&utcb_area_attr,
            ROUND_UP(pd->_init.utcb_area_virt_item.base, utcb_area_size),
            utcb_area_size);
    okl4_utcb_area_init(pd->_init.utcb_area, &utcb_area_attr);

    return 0;
}

/*
 * Destruct the okl4_utcb_area_t created by the pd.
 */
static void
destruct_utcb_area(okl4_pd_t *pd)
{
    assert(pd->_init.virtmem_pool != NULL);

    okl4_virtmem_pool_free(pd->_init.virtmem_pool,
            &pd->_init.utcb_area_virt_item);
}

/*
 * Construct a kthread pool allocator using memory reserved at pd creation time.
 */

static void
construct_kthread_pool(okl4_pd_t *pd, okl4_pd_attr_t *attr)
{
    okl4_allocator_attr_t thread_alloc_attr;

    assert(pd->thread_pool != NULL);
    assert(pd->thread_alloc != NULL);

    /* Initialise the kthread memory allocator. */
    okl4_allocator_attr_init(&thread_alloc_attr);
    okl4_allocator_attr_setsize(&thread_alloc_attr, attr->max_threads);

    okl4_bitmap_allocator_init(pd->thread_alloc, &thread_alloc_attr);
}

/*
 * Construct a okl4_kspace_t using pools provided by the user.
 */
static int
construct_kspace(okl4_pd_t *pd, okl4_pd_attr_t *attr)
{
    okl4_kspace_attr_t kspace_attr;
    int error;

    assert(pd->kspace != NULL);
    assert(pd->_init.kspaceid_pool != NULL);

    /* Allocate a KSpace ID. */
    error = okl4_kspaceid_allocany(pd->_init.kspaceid_pool,
            &pd->_init.kspace_id);
    if (error) {
        return error;
    }

    /* Set the kspace attributes */
    okl4_kspace_attr_init(&kspace_attr);
    okl4_kspace_attr_setkclist(&kspace_attr, pd->_init.kclist);
    okl4_kspace_attr_setid(&kspace_attr, pd->_init.kspace_id);
    okl4_kspace_attr_setutcbarea(&kspace_attr, pd->_init.utcb_area);
    okl4_kspace_attr_setprivileged(&kspace_attr, attr->privileged);
    error = okl4_kspace_create(&pd->_init.kspace_mem, &kspace_attr);
    if (error) {
        okl4_kspaceid_free(pd->_init.kspaceid_pool, pd->_init.kspace_id);
        return error;
    }

    return 0;
}

/*
 * Construct a kspace object using the pools provided by the user.
 */
static int
construct_pd_from_pools(okl4_pd_t *pd, okl4_pd_attr_t *attr)
{
    int error;

    /* Ensure all attributes are setup correctly. */
    assert(attr->kspaceid_pool != NULL);
    assert(attr->virtmem_pool != NULL);
    assert(!(attr->kclist == NULL && attr->kclistid_pool == NULL));

    /* Construct a kclist. */
    error = construct_kclist(pd, attr);
    if (error) {
        return error;
    }

    /* Construct a UTCB area. */
    error = construct_utcb_area(pd, attr);
    if (error) {
        destruct_kclist(pd);
        return error;
    }

    /* Construct the kspace. */
    error = construct_kspace(pd, attr);
    if (error) {
        destruct_kclist(pd);
        destruct_utcb_area(pd);
        return error;
    }

    return 0;
}

/*
 * Lookup mappings in the PD's UTCB area. No mappings should be performed,
 * as the kernel should provide these.
 */
int
_okl4_utcb_memsec_lookup(okl4_memsec_t *memsec, okl4_word_t vaddr,
        okl4_physmem_item_t *map_item, okl4_word_t *dest_vaddr)
{
    /* Don't perform any map operations on this memsec. */
    return 1;
}

/*
 * Handle a fault in the UTCB area. No faults should every occur
 * in the UTCB area, as the kernel should handle the mappings.
 */
int
_okl4_utcb_memsec_map(okl4_memsec_t *memsec, okl4_word_t vaddr,
        okl4_physmem_item_t *map_item, okl4_word_t *dest_vaddr)
{

    /* GNU C complains if we don't have this return, flint complains if we do. */
    assert(!"Faulted in UTCB area."); /*lint -e527 */
    return 1; /*lint -e527 */
}

void
_okl4_utcb_memsec_destroy(okl4_memsec_t *ms)
{
    /* No action taken. */
}

/*
 * Create a dummy UTCB memsec to ensure that the UTCB is not mapped
 * over.
 */
static void
create_utcb_memsec(okl4_pd_t *pd, okl4_utcb_area_t *utcb_area)
{
    int error;
    okl4_range_item_t utcb_vrange;
    okl4_memsec_attr_t attr;

    assert(pd != NULL);

    /* Set range to UTCB area. */
    okl4_memsec_attr_init(&attr);
    utcb_vrange.next = NULL;
    utcb_vrange.total_size = 0;
    okl4_range_item_setrange(&utcb_vrange, utcb_area->utcb_memory.base,
            ROUND_UP(utcb_area->utcb_memory.size, OKL4_DEFAULT_PAGESIZE));
    okl4_memsec_attr_setrange(&attr, utcb_vrange);

    /* Setup dummy callbacks. */
    okl4_memsec_attr_setpremapcallback(&attr, _okl4_utcb_memsec_lookup);
    okl4_memsec_attr_setaccesscallback(&attr, _okl4_utcb_memsec_map);
    okl4_memsec_attr_setdestroycallback(&attr, _okl4_utcb_memsec_destroy);

    /* Create the memsec. */
    okl4_memsec_init(&pd->utcb_memsec, &attr);

    /* Attach. */
    error = okl4_pd_memsec_attach(pd, &pd->utcb_memsec);
    assert(!error);
}

/**
 * Initialise a protection domain from an attributes object.
 */
int
okl4_pd_create(okl4_pd_t *pd, okl4_pd_attr_t *attr)
{
    int error;

    assert(pd != NULL);
    assert(attr != NULL);

    /*
     * Ensure that the attributes object contains valid values.
     *
     * These asserts are a little more strict than is strictly necessary: The
     * library could choose to be smart when conflicting attributes are given,
     * arbitrarily choosing a sane option. We instead elect to be stricter on
     * inputs, hoping that this will catch when the user is misusing the API.
     */
    if (attr->kspace) {
        /* If the user has provided us with a pre-constructed kspace, then no
         * pools are required. Also they should not have provided a kclist
         * because this is included in the given kspace. */
        assert(!attr->virtmem_pool);
        assert(!attr->kclistid_pool);
        assert(!attr->kspaceid_pool);
        assert(!attr->kclist);
    } else if (attr->kclist) {
        /* If the user provided us with a kclist then they must not pass in a
         * clistid_pool, but should pass in everything else. */
        assert(!attr->kclistid_pool);
        assert(attr->virtmem_pool);
        assert(attr->kspaceid_pool);
        assert(attr->kclist_size > 0);
    } else {
        /* Otherwise, the user must give us all the pools we need to construct
         * a kspace. */
        assert(attr->virtmem_pool);
        assert(attr->kclistid_pool);
        assert(attr->kspaceid_pool);
        assert(attr->kclist_size > 0);
    }

    /* Check other attributes are valid. */
    assert(attr->max_threads > 0);

    /* Setup the slack memory in this okl4_pd_t struct. */
    allocate_pd_slack_memory(pd, attr);

    /* Setup the protection domain struct. */
    pd->mem_container_list = NULL;
    pd->root_kclist = attr->root_kclist;
    pd->dict_next = NULL;
    pd->extension = NULL;
    pd->default_pager = attr->default_pager;

    /* Setup a kspace. */
    if (attr->kspace != NULL) {
        /* If the user specified a kspace, just use that. */
        pd->_init.virtmem_pool = NULL;
        pd->_init.kclistid_pool = NULL;
        pd->_init.kspaceid_pool = NULL;
        pd->_init.kclist_id.raw = ~0UL;
        pd->kspace = attr->kspace;
    } else if (attr->kclist != NULL) {
        /* If the user specified a kclist then don't use clistid pool. */
        pd->_init.virtmem_pool = attr->virtmem_pool;
        pd->_init.kclistid_pool = NULL;
        pd->_init.kclist_id.raw = ~0UL;
        pd->_init.kclist = attr->kclist;
        pd->_init.kspaceid_pool = attr->kspaceid_pool;
        error = construct_pd_from_pools(pd, attr);
        if (error) {
            return error;
        }
    } else {
        /* If the user didn't provide us with a kspace and we are not
         * constructing the root PD, construct the PD from provided pools. */
        pd->_init.virtmem_pool = attr->virtmem_pool;
        pd->_init.kclistid_pool = attr->kclistid_pool;
        pd->_init.kspaceid_pool = attr->kspaceid_pool;
        error = construct_pd_from_pools(pd, attr);
        if (error) {
            return error;
        }
    }

    /* Construct PD's pool of kthreads. */
    construct_kthread_pool(pd, attr);

    /* Create a dummy UTCB memsec to ensure that the UTCB is not mapped
     * over. */
    create_utcb_memsec(pd, pd->kspace->utcb_area);

    return 0;
}

