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
#if defined(ARM_SHARED_DOMAINS)
#include <l4/map.h>
#include <l4/space.h>
#endif

#include <okl4/types.h>
#include <okl4/pd.h>
#include <okl4/memsec.h>
#include <okl4/zone.h>
#include <okl4/extension.h>

#include "pd_helpers.h"
#include "zone_helpers.h"

/**
 * Detach the given zone from the pd.
 */
void
okl4_pd_zone_detach(okl4_pd_t *pd, okl4_zone_t *zone)
{
    okl4_word_t i;
    _okl4_mcnode_t *zone_node;

    assert(pd != NULL);
    assert(zone != NULL);

    /* Remove zone from list of memory containers attached to the pd. Returns a
     * pointer to the node that this zone was held . */
    zone_node = _okl4_mem_container_list_remove(&pd->mem_container_list,
            &zone->super);
    assert(zone_node != NULL);

    /* Free the node used to carry this zone in the pd list of zones. */
    _okl4_zone_mcnode_free(zone, zone_node);

    /* Remove ourself from the list of PDs attached to the zone. */
    for (i = 0; i < zone->num_parent_pds; i++) {
        if (zone->parent_pds[i] == pd) {
            break;
        }
    }
    assert(i < zone->num_parent_pds);
    zone->parent_pds[i] = zone->parent_pds[zone->num_parent_pds - 1];
    zone->num_parent_pds--;

#if defined(ARM_SHARED_DOMAINS)
    /* FIXME: This does not appear to work. Should unmap window? */
    _okl4_kspace_domain_unshare(pd->kspace->id, zone->kspaceid);
#else
    /* Unmap the zone from the pd. */
    _okl4_kspace_unmap_zone(pd->kspace->id, zone);
#endif

    /* If we have an extension, unmap it from there too. */
    if (pd->extension != NULL) {
        _okl4_kspace_unmap_zone(pd->extension->kspace->id, zone);
    }
}

