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

#include <okl4/types.h>
#include <okl4/pd.h>
#include <okl4/memsec.h>
#include <okl4/zone.h>
#include <okl4/extension.h>

#include "pd_helpers.h"
#include "zone_helpers.h"

/*
 * Perform the attach, assuming data structures have already been updated.
 */
static void
do_attach_zone(okl4_pd_t *pd, okl4_zone_t *zone, okl4_word_t perms)
{
#if defined(ARM_SHARED_DOMAINS)
    int error;

    /* For ARM shared domains, we only support attach permissions of
     * L4_FullyAccessible. */
    assert(perms == L4_FullyAccessible);

    /* Share the zone space. */
    error = _okl4_kspace_share_domain_add_window(pd->kspace->id,
            zone->kspaceid, 0, zone->super.range);
    assert(!error);

    /* If we have a extension, we also need to add a window into the
     * extension space. */
    if (pd->extension != NULL) {
        error = _okl4_kspace_share_domain_add_window(pd->extension->kspace->id,
                zone->kspaceid, 0, zone->super.range);
        assert(!error);
    }
#else
    /* Premap the zone into the pd. */
    _okl4_kspace_premap_zone(pd->kspace->id, zone, perms);

    /* If we have an extension, premap it into there as well. */
    if (pd->extension != NULL) {
        _okl4_extension_premap_mem_container(pd->extension,
                (_okl4_mem_container_t *)zone, perms);
    }
#endif
}

/**
 * Attach the given zone to the pd.
 */
int
okl4_pd_zone_attach(okl4_pd_t *pd, okl4_zone_t *zone,
        okl4_pd_zone_attach_attr_t *attr)
{
    int error;
    _okl4_mcnode_t *zone_node;

    assert(pd != NULL);
    assert(zone != NULL);
    assert(attr != NULL);

    /* Have to add this zone to our list of zones. Since each zone can be
     * attached to any number of pds a single 'next' pointer in the zone struct
     * is not sufficient. Each zone must have a collection of 'next' pointers.
     * First step is to allocate one of these. */
    error = _okl4_zone_mcnode_alloc(zone, &zone_node);
    if (error) {
        return error;
    }

    /* Insert zone node into pd list of memory containers. */
    zone_node->payload = &zone->super;
    zone_node->attach_perms = attr->perms;
    error = _okl4_mem_container_list_insert(&pd->mem_container_list,
            zone_node);
    if (error) {
        return error;
    }

    /* Add this PD into the list of PDs attached the zone. */
    assert(zone->num_parent_pds < zone->max_pds);
    zone->parent_pds[zone->num_parent_pds] = pd;
    zone->num_parent_pds++;

    /* Perform the attach. */
    do_attach_zone(pd, zone, attr->perms);

    return OKL4_OK;
}

