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

#include <okl4/zone.h>
#include <okl4/bitmap.h>
#include "pd_helpers.h"

#if defined(ARM_SHARED_DOMAINS)
/*
 * Construct a space.
 */
static int
construct_space(okl4_zone_t *zone)
{
    okl4_kclistid_t null_clist = {0};
    int error;
    okl4_word_t result;

    assert(zone != NULL);

    /* If the zone has a valid kspace id pool then allocate the id
     * from there, otherwise assume the zone already has a one. */
    if (zone->kspaceid_pool != NULL) {
        error = okl4_kspaceid_allocany(zone->kspaceid_pool, &zone->kspaceid);
        if (error) {
            return error;
        }
    }

    result = L4_SpaceControl(zone->kspaceid, L4_SpaceCtrl_new,
            null_clist, L4_Nilpage, 0, NULL);
    if (result != 1) {
        if (zone->kspaceid_pool != NULL) {
            okl4_kspaceid_free(zone->kspaceid_pool, zone->kspaceid);
        }
        return _okl4_convert_kernel_errno(L4_ErrorCode());
    }

    return OKL4_OK;
}
#endif

int
okl4_zone_create(okl4_zone_t *zone, okl4_zone_attr_t *attr)
{
    /* We flagrantly abuse memory in this function. */
    /*lint --e{826}*/

    okl4_allocator_attr_t bitmap_attr;
    char *location;
#if defined(ARM_SHARED_DOMAINS)
    int error;
#endif

    assert(zone != NULL);
    assert(attr != NULL);

    /* Check zone alignment and size. */
    assert(attr->range.base % OKL4_ZONE_ALIGNMENT == 0);
    assert(attr->range.size % OKL4_ZONE_ALIGNMENT == 0);

    /* Initialise zone struct */
    zone->num_parent_pds = 0;
    zone->mem_container_list = NULL;

    /* Copy attributes to the zone struct. */
    zone->max_pds = attr->max_pds;
    zone->super.range = attr->range;
    zone->super.type = _OKL4_TYPE_ZONE;

#if defined(ARM_SHARED_DOMAINS)
    /* The attributes must contain either a kspace or a space id pool, but not
     * both. */
    assert(XOR(attr->kspaceid.raw != ~0UL, attr->kspaceid_pool));

    zone->kspaceid_pool = attr->kspaceid_pool;
    zone->kspaceid = attr->kspaceid;

    error = construct_space(zone);
    if (error) {
        return error;
    }
#endif

    /* Setup the free node pointer and allocator. Note: this could be done in a
     * more space-efficient manner by maintaining a free list of memory
     * container nodes, rather than using a bitmap allocator. */
    location = (char *)zone + sizeof(okl4_zone_t);
    zone->mcnode_pool = (_okl4_mcnode_t *)location;
    location += _OKL4_ZONE_MCNODE_POOL_SIZE(attr->max_pds);

    zone->mcnode_alloc = (okl4_bitmap_allocator_t *)location;
    location += OKL4_BITMAP_ALLOCATOR_SIZE(attr->max_pds);

    zone->parent_pds = (okl4_pd_t **)location;
    location += sizeof(okl4_pd_t *) * attr->max_pds;

    /* Ensure size was correct. */
    assert((unsigned int)(location - (char *)zone) == OKL4_ZONE_SIZE_ATTR(attr));

    /* Initialise the allocator. */
    okl4_allocator_attr_init(&bitmap_attr);
    okl4_allocator_attr_setrange(&bitmap_attr, 0, attr->max_pds);
    okl4_bitmap_allocator_init(zone->mcnode_alloc, &bitmap_attr);

    return OKL4_OK;
}

