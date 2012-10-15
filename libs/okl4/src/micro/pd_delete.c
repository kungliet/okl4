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
#include <l4/types.h>

#include <okl4/types.h>
#include <okl4/pd.h>
#include <okl4/kspace.h>
#include <okl4/memsec.h>
#include <okl4/zone.h>

#include "pd_helpers.h"

/*
 * Destruct the okl4_kclist_t created by the pd.
 */
static void
destruct_kclist(okl4_pd_t *pd)
{
    /* If the user gave us the kclist in the first place, we must not destroy
     * it. */
    if (pd->_init.kclist_id.raw == ~0UL) {
        return;
    }

    /* Otherwise, delete the kclist and free the identifier. */
    assert(pd->_init.kclistid_pool != NULL);
    okl4_kclist_delete(pd->_init.kclist);
    okl4_kclistid_free(pd->_init.kclistid_pool, pd->_init.kclist_id);
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
 * Destruct the kthread pool created by the pd. This does not free any memory,
 * just zero's the corrosponding bitmap allocator.
 */
static void
destruct_kthread_pool(okl4_pd_t *pd)
{
    okl4_bitmap_allocator_destroy(pd->thread_alloc);
}

/*
 * Destruct the okl4_kspace_t created by the pd.
 */
static void
destruct_kspace(okl4_pd_t *pd)
{
    assert(pd->_init.kspaceid_pool != NULL);

    okl4_kspace_delete(pd->kspace);
    okl4_kspaceid_free(pd->_init.kspaceid_pool, pd->_init.kspace_id);
}

/*
 * Destroy a kspace created from individual pools.
 */
static void
destruct_pd_from_pools(okl4_pd_t *pd)
{
    destruct_kspace(pd);
    destruct_utcb_area(pd);
    destruct_kclist(pd);
    destruct_kthread_pool(pd);
}

/*
 * Determine if the given PD was created from pools.
 */
static int
is_pool_created_pd(okl4_pd_t *pd)
{
    /* We were created from pools iff our kspace pointer points
     * inside our PD. */
    int made_from_pool = pd->kspace == &pd->_init.kspace_mem;

    if (made_from_pool) {
        assert(pd->_init.kspaceid_pool != NULL);
        assert(pd->_init.virtmem_pool != NULL);
    } else {
        assert(pd->_init.kclistid_pool == NULL);
        assert(pd->_init.kclist_id.raw == ~0UL);
        assert(pd->_init.kspaceid_pool == NULL);
        assert(pd->_init.virtmem_pool == NULL);
    }

    return made_from_pool;
}

/**
 * Destroy a protection domain, detaching all memsecs currently attached to it.
 */
void
okl4_pd_delete(okl4_pd_t *pd)
{
    assert(pd != NULL);

    /* Can only delete if there are no memsections/zones/extensions (other than
     * the utcb memsec) currently attached. */
    assert(pd->mem_container_list->payload ==
            (_okl4_mem_container_t *)&pd->utcb_memsec
                && (pd->mem_container_list->next == NULL));

    /* Delete any threads that were created by us. */
    while (pd->kspace->kthread_list) {
        okl4_pd_thread_delete(pd, pd->kspace->kthread_list);
    }

    /* If we were created from pools, destroy us. */
    if (is_pool_created_pd(pd)) {
        destruct_pd_from_pools(pd);
    }
}
