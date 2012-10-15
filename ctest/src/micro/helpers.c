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

#include <okl4/memsec.h>
#include <okl4/zone.h>
#include <okl4/static_memsec.h>
#include <okl4/physmem_item.h>
#include <okl4/physmem_segpool.h>
#include <okl4/pd.h>
#include <okl4/kspace.h>
#include <okl4/bitmap.h>
#include <okl4/kclist.h>
#include <l4/misc.h>

#include "../helpers.h"

/*
 * Create a static memsec with the given base virtual address and the given
 * size. If base address is -1 then it is ignored and any free virtual address
 * is chosen.
 *
 * Allocate the virtual range for the memsection from the given virtmem pool.
 * If this is null then use the root_virtmem_pool.
 *
 * If virtmem_pool is the magic constant "NO_POOL", don't use a pool at all. In
 * this case, both base and size must be specified.
 *
 * To ease implementation of zone and extension tests, memsecs allocated out
 * of default pools are always OKL4_ZONE_ALIGNMENT aligned.
 */
ms_t *
create_memsec(okl4_word_t base, okl4_word_t size,
              okl4_virtmem_pool_t *virtmem_pool)
{
    return create_memsec_with_perms(base, size, virtmem_pool,
                                    L4_Readable | L4_Writable);
}

static int
allocate_virt_pool(okl4_word_t base, okl4_word_t size,
        okl4_virtmem_pool_t *pool, okl4_virtmem_item_t *item,
        okl4_virtmem_item_t *allocation)
{
    okl4_word_t allocation_size;
    int result;

    /* If we are not using a pool, just setup the item. */
    if (pool == NO_POOL) {
        assert(base != ANONYMOUS_BASE);
        item->base = base;
        item->size = size;
        return 0;
    }

    /* Use the root pool if no pool was specified. */
    if (pool == NULL) {
        pool = root_virtmem_pool;
    }

    /* If the user specified a base, just perform a simple allocation. */
    if (base != ANONYMOUS_BASE) {
        okl4_range_item_setrange(allocation, base, size);
        result = okl4_virtmem_pool_alloc(pool, allocation);
        *item = *allocation;
        return result;
    }

    /* Otherwise, setup the allocation item to be sufficiently large
     * so that we can find a OKL4_ZONE_ALIGNMENT aligned chunk of space. */
    allocation_size = size;
    if (allocation_size % OKL4_ZONE_ALIGNMENT > 0) {
        allocation_size -= (allocation_size % OKL4_ZONE_ALIGNMENT);
        allocation_size += OKL4_ZONE_ALIGNMENT;
    }
    allocation_size *= 2;

    /* Perform the allocation. */
    okl4_range_item_setsize(allocation, allocation_size);
    result = okl4_virtmem_pool_alloc(pool, allocation);
    if (result) {
        return result;
    }

    /* Setup the return item to pretend they only got the amount of memory
     * they asked for. */
    item->base = allocation->base;
    if (item->base % OKL4_ZONE_ALIGNMENT != 0) {
        item->base -= (item->base % OKL4_ZONE_ALIGNMENT);
        item->base += OKL4_ZONE_ALIGNMENT;
    }
    item->size = size;

    return 0;
}

ms_t *
create_memsec_with_perms(okl4_word_t base, okl4_word_t size, 
              okl4_virtmem_pool_t *virtmem_pool, okl4_word_t perms)
{
    int ret;
    okl4_static_memsec_attr_t attr;

    /* Allocate the memsec. */
    ms_t *ms = malloc(sizeof(ms_t));
    assert(ms);

    /* Pull virtual address range out of the pool. */
    ret = allocate_virt_pool(base, size, virtmem_pool, &ms->virt,
            &ms->allocated_virt);
    assert(ret == 0);

    /* Pull physical range out of the pool. */
    okl4_physmem_item_setsize(&ms->phys, size);
    ret = okl4_physmem_segpool_alloc(root_physseg_pool, &ms->phys);
    assert(ret == 0);

    /* Create the memsec. */
    okl4_static_memsec_attr_init(&attr);
    okl4_static_memsec_attr_setperms(&attr, perms);
    okl4_static_memsec_attr_setattributes(&attr, L4_DefaultMemory);
    okl4_static_memsec_attr_setrange(&attr, ms->virt);
    okl4_static_memsec_attr_settarget(&attr, ms->phys);
    okl4_static_memsec_init(&ms->static_memsec, &attr);

    /* Setup the memsec pointer. */
    ms->memsec = okl4_static_memsec_getmemsec(&ms->static_memsec);

    return ms;
}

static int
_lookup(okl4_memsec_t *memsec, okl4_word_t vaddr,
        okl4_physmem_item_t *map_item, okl4_word_t *dest_vaddr)
{
    /* Don't perform any eager mapping. */
    printf("lookup callback on %08x\n", (int)vaddr);
    return 1;
}

static int
_map(okl4_memsec_t *memsec, okl4_word_t vaddr,
        okl4_physmem_item_t *map_item, okl4_word_t *dest_vaddr)
{
    int error;
    okl4_physmem_item_t *phys;

    /* Allocate a page. */
    printf("map callback on %08x\n", (int)vaddr);
    phys = malloc(sizeof(okl4_physmem_item_t));
    assert(phys);
    okl4_physmem_item_setsize(phys, OKL4_DEFAULT_PAGESIZE);
    error = okl4_physmem_segpool_alloc(root_physseg_pool, phys);
    assert(!error);

    /* Give that back to the user. */
    *dest_vaddr = vaddr & ~(OKL4_DEFAULT_PAGESIZE - 1);
    *map_item = *phys;

    return 0;
}

static void
_destroy(okl4_memsec_t *memsec)
{
}

okl4_memsec_t *
create_paged_memsec(okl4_virtmem_item_t virt_item)
{
    okl4_memsec_attr_t attr;
    okl4_virtmem_item_t range;
    okl4_memsec_t *memsec;

    /* Setup the memsec attributes. */
    okl4_range_item_setrange(&range, virt_item.base,
            OKL4_DEFAULT_PAGESIZE);
    okl4_memsec_attr_init(&attr);
    okl4_memsec_attr_setrange(&attr, range);
    okl4_memsec_attr_setpremapcallback(&attr, _lookup);
    okl4_memsec_attr_setaccesscallback(&attr, _map);
    okl4_memsec_attr_setdestroycallback(&attr, _destroy);

    /* Create the memsec. */
    memsec = malloc(OKL4_MEMSEC_SIZE_ATTR(&attr));
    assert(memsec);
    okl4_memsec_init(memsec, &attr);
    return memsec;
}


/*
 * Delete a memsec. Free the virtual range to the given pool - or to the root
 * pool if none is given.
 */
void
delete_memsec(ms_t *ms, okl4_virtmem_pool_t *virtmem_pool)
{
    okl4_memsec_destroy(ms->memsec);
    okl4_physmem_segpool_free(root_physseg_pool, &ms->phys);
    if (virtmem_pool == NO_POOL) {
        /* Do nothing. */
    } else if (virtmem_pool != NULL) {
        okl4_virtmem_pool_free(virtmem_pool, &ms->allocated_virt);
    } else {
        okl4_virtmem_pool_free(root_virtmem_pool, &ms->allocated_virt);
    }
    free(ms);
}

/*
 * Create a zone with the given base virtual address and the given size.
 */
okl4_zone_t *
create_zone(okl4_word_t base, okl4_word_t size)
{
    int error;
    okl4_zone_attr_t attr;
    okl4_virtmem_item_t virt;
#if defined(ARM_SHARED_DOMAINS)
    okl4_kspaceid_t kspaceid;
#endif
    okl4_zone_t *zone;

    /* Create the zone. */
    okl4_range_item_setrange(&virt, base, size);
    okl4_zone_attr_init(&attr);
    okl4_zone_attr_setrange(&attr, virt);
    okl4_zone_attr_setmaxpds(&attr, 10);
#if defined(ARM_SHARED_DOMAINS)
    error = okl4_kspaceid_allocany(root_kspace_pool, &kspaceid);
    fail_unless(error == 0, "Failed to allocate a kspace id.");
    okl4_zone_attr_setkspaceid(&attr, kspaceid);
#endif

    zone = malloc(OKL4_ZONE_SIZE_ATTR(&attr));
    assert(zone);

    error = okl4_zone_create(zone, &attr);
    fail_unless(error == 0, "Could not create zone.");

    return zone;
}

/*
 * Delete a zone created by 'create_zone'.
 */
void
delete_zone(okl4_zone_t *zone)
{
    okl4_zone_delete(zone);
    free(zone);
}

/*
 * Create a basic kspace object.
 */
ks_t *
create_kspace(void)
{
    int ret;
    okl4_kclist_attr_t kclist_attr;
    okl4_utcb_area_attr_t utcb_area_attr;
    okl4_kspace_attr_t kspace_attr;
    ks_t *ks;

    /* Allocate a ks object. */
    ks = malloc(sizeof(struct ks));
    assert(ks);

    /* Allocate a kclist ID. */
    ret = okl4_kclistid_allocany(root_kclist_pool, &ks->kclist_id);
    fail_unless(ret == 0, "Failed to allocate a kclist.");

    /* Create the kclist object. */
    okl4_kclist_attr_init(&kclist_attr);
    okl4_kclist_attr_setid(&kclist_attr, ks->kclist_id);
    ks->kclist = malloc(OKL4_KCLIST_SIZE_ATTR(&kclist_attr));
    assert(ks->kclist);
    ret = okl4_kclist_create(ks->kclist, &kclist_attr);
    fail_unless(ret == 0, "Failed to create a kclist.");

    /* Create a UTCB area. */
    okl4_utcb_area_attr_init(&utcb_area_attr);
    okl4_utcb_area_attr_setrange(&utcb_area_attr,
            UTCB_ADDR, UTCB_SIZE * MAX_THREADS);
    ks->utcb_area = malloc(OKL4_UTCB_AREA_SIZE_ATTR(&utcb_area_attr));
    assert(ks->utcb_area);
    okl4_utcb_area_init(ks->utcb_area, &utcb_area_attr);

    /* Allocate a KSpace ID. */
    ret = okl4_kspaceid_allocany(root_kspace_pool, &ks->kspace_id);
    fail_unless(ret == 0, "Failed to allocate a kspace.");

    /* Set the kspace attributes */
    okl4_kspace_attr_init(&kspace_attr);
    okl4_kspace_attr_setkclist(&kspace_attr, ks->kclist);
    okl4_kspace_attr_setid(&kspace_attr, ks->kspace_id);
    okl4_kspace_attr_setutcbarea(&kspace_attr, ks->utcb_area);
    okl4_kspace_attr_setprivileged(&kspace_attr, 1);

    /* Allocate the kspace  */
    ks->kspace = malloc(OKL4_KSPACE_SIZE_ATTR(&kspace_attr));
    assert(ks->kspace);

    /* Create the kspace */
    ret = okl4_kspace_create(ks->kspace, &kspace_attr);
    fail_unless(ret == OKL4_OK, "Failed to create a kspace");

    return ks;
}

void
delete_kspace(ks_t *ks)
{
    okl4_kspace_delete(ks->kspace);
    okl4_kclist_delete(ks->kclist);

    okl4_kclistid_free(root_kclist_pool, ks->kclist_id);
    okl4_kspaceid_free(root_kspace_pool, ks->kspace_id);

    free(ks->kclist);
    free(ks->kspace);
    free(ks->utcb_area);
    free(ks);
}

okl4_pd_t *
create_pd(void)
{
    int error;
    okl4_pd_t *pd;
    okl4_pd_attr_t pd_attr;

     /* Create a PD. */
    okl4_pd_attr_init(&pd_attr);
    okl4_pd_attr_setkspaceidpool(&pd_attr, root_kspace_pool);
    okl4_pd_attr_setkclistidpool(&pd_attr, root_kclist_pool);
    okl4_pd_attr_setutcbmempool(&pd_attr, root_virtmem_pool);
    okl4_pd_attr_setrootkclist(&pd_attr, root_kclist);
    okl4_pd_attr_setmaxthreads(&pd_attr, MAX_THREADS);
    okl4_pd_attr_setprivileged(&pd_attr, 1);

    pd = malloc(OKL4_PD_SIZE_ATTR(&pd_attr));
    assert(pd);

    error = okl4_pd_create(pd, &pd_attr);
    fail_unless(error == OKL4_OK, "Could not create PD");

    return pd;
}

void
delete_pd(okl4_pd_t *pd)
{
    okl4_pd_delete(pd);
    free(pd);
}

okl4_kthread_t *
run_thread_in_pd(okl4_pd_t *pd, okl4_word_t sp, okl4_word_t ip)
{
    int error;
    okl4_kthread_t *thread;
    okl4_pd_thread_create_attr_t thread_attr;

    okl4_pd_thread_create_attr_init(&thread_attr);
    okl4_pd_thread_create_attr_setspip(&thread_attr, sp, ip);
    okl4_pd_thread_create_attr_setpriority(&thread_attr, 102);
    okl4_pd_thread_create_attr_setpager(&thread_attr,
            okl4_kthread_getkcap(root_thread));
    error = okl4_pd_thread_create(pd, &thread_attr, &thread);
    fail_unless(error == OKL4_OK, "Could not create thread");

    okl4_pd_thread_start(pd, thread);

    return thread;
}

okl4_extension_t *
create_extension(okl4_pd_t *base_pd)
{
    int error;
    okl4_extension_t *ext;
    okl4_extension_attr_t attr;
    okl4_virtmem_item_t virt;

    /* Setup the extension. */
    okl4_extension_attr_init(&attr);
    okl4_range_item_setrange(&virt, EXTENSION_BASE_ADDR, EXTENSION_SIZE);
    okl4_extension_attr_setrange(&attr, virt);
    okl4_extension_attr_setbasepd(&attr, base_pd);
    okl4_extension_attr_setkspaceidpool(&attr, root_kspace_pool);
    ext = malloc(OKL4_EXTENSION_SIZE_ATTR(&attr));
    assert(ext);
    error = okl4_extension_create(ext, &attr);
    fail_unless(error == 0, "Could not create extension.");

    return ext;
}

/*
 * Attach the given zone to the given protection domain, using default
 * permissions.
 */
void
attach_zone(okl4_pd_t *pd, okl4_zone_t *zone)
{
    attach_zone_perms(pd, zone, ~0);
}

/*
 * Attach the given zone to the given protection domain, using the provided
 * permissions.
 */
void
attach_zone_perms(okl4_pd_t *pd, okl4_zone_t *zone, okl4_word_t perms)
{
    int error;
    okl4_pd_zone_attach_attr_t attr;

    okl4_pd_zone_attach_attr_init(&attr);
    if (perms != (okl4_word_t)-1) {
        okl4_pd_zone_attach_attr_setperms(&attr, perms);
    }
    error = okl4_pd_zone_attach(pd, zone, &attr);
    fail_unless(error == 0, "Could not attach zone to PD.");
}

/*
 * Get a range of virtual memory out of the default pool.
 */
okl4_range_item_t *
get_virtual_address_range(okl4_word_t size)
{
    int error;
    okl4_range_item_t *item;

    /*
     * Allocate two range items, side-by-size. The first one is what the user
     * sees, and the second one is the item that was actually used for the
     * allocation.
     *
     * This is necessary because of the less-than-optimial interface of
     * 'allocate_virt_pool'.
     */
    item = malloc(sizeof(okl4_range_item_t) * 2);
    assert(item);

    /* Allocate memory. */
    error = allocate_virt_pool(ANONYMOUS_BASE, size, root_virtmem_pool,
            &item[0], &item[1]);
    assert(!error);

    return item;
}

void
free_virtual_address_range(okl4_range_item_t *item)
{
    okl4_virtmem_pool_free(root_virtmem_pool, &item[1]);
    free(item);
}

void
send_ipc(L4_ThreadId_t dest)
{
    L4_MsgTag_t tag;
    L4_Msg_t msg;
    L4_MsgClear(&msg);
    L4_MsgLoad(&msg);
    tag = L4_Send(dest);
    fail_unless(L4_IpcSucceeded(tag) != 0, "L4_Send() failed.\n");
}


