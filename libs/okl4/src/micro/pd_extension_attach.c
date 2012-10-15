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

#include <l4/thread.h>
#include <l4/space.h>
#include <l4/kdebug.h>

#include <okl4/zone.h>
#include <okl4/types.h>
#include <okl4/memsec.h>
#include <okl4/extension.h>
#include <okl4/kspace.h>

#include "pd_helpers.h"

#ifdef ARM_SHARED_DOMAINS
static void
map_window(okl4_kspaceid_t client, okl4_kspaceid_t owner,
        okl4_range_item_t range)
{
    /* Align outwards to 1MB boundaries. */
    range.size += (range.base % OKL4_EXTENSION_ALIGNMENT);
    range.base -= (range.base % OKL4_EXTENSION_ALIGNMENT);
    if (range.size % OKL4_EXTENSION_ALIGNMENT > 0) {
        range.size -= (range.size % OKL4_EXTENSION_ALIGNMENT);
        range.size += OKL4_EXTENSION_ALIGNMENT;
    }

    /*
     * Create the window.
     *
     * We don't check to see if there was an error: If there is an
     * error, we assume it is because of overmapping another window we
     * have already set up.
     */
    (void)_okl4_kspace_share_domain_add_window(client, owner, 0, range);
}
#endif

void
_okl4_extension_premap_mem_container(okl4_extension_t *ext,
        _okl4_mem_container_t *container, okl4_word_t attach_perms)
{
#ifndef ARM_SHARED_DOMAINS

    if (container->type == _OKL4_TYPE_MEMSECTION) {
        _okl4_kspace_premap_memsec(ext->kspace->id,
                (okl4_memsec_t *)container, attach_perms);
    } else if (container->type == _OKL4_TYPE_ZONE) {
        _okl4_kspace_premap_zone(ext->kspace->id,
                (okl4_zone_t *)container, attach_perms);
    } else {
        assert(!"Unknown container type.");
    }

#else /* ARM_SHARED_DOMAINS */

    /* Map a window from the extension space back into the base space. */
    if (container->type == _OKL4_TYPE_MEMSECTION) {
        map_window(ext->kspace->id, ext->base_pd->kspace->id, container->range);
    } else if (container->type == _OKL4_TYPE_ZONE) {
        map_window(ext->kspace->id, ((okl4_zone_t *)container)->kspaceid,
                container->range);
    } else {
        assert(!"Unknown container type.");
    }

#endif /* ARM_SHARED_DOMAINS */
}

/*
 * Premap every memsec already attached to the protection domain into our
 * extension space.
 */
static void
premap_extension_space(okl4_pd_t *pd, okl4_extension_t *ext)
{
    _okl4_mcnode_t *next = pd->mem_container_list;

    while (next != NULL) {
        if (next->payload != &ext->super) {
            _okl4_extension_premap_mem_container(ext,
                    next->payload, next->attach_perms);
        } else {
            assert(!"Unknown memory container type.");
        }
        next = next->next;
    }
}

/*
 * Attach the given extension to the given pd.
 */
int
okl4_pd_extension_attach(okl4_pd_t *pd, okl4_extension_t *ext)
{
    int error;

    /* Ensure that the extension is appropriate for the PD. */
    if (ext->base_pd != pd) {
        return OKL4_INVALID_ARGUMENT;
    }

    /* Ensure that no other extension is already attached. */
    if (pd->extension != NULL) {
        return OKL4_IN_USE;
    }

    /* Populate the extension space with the mappings in the base space. */
    premap_extension_space(pd, ext);
    /* Add this extension to the PD. */
    ext->node.payload = &ext->super;
    ext->node.attach_perms = L4_FullyAccessible;
    error = _okl4_mem_container_list_insert(&pd->mem_container_list,
            &ext->node);
    if (error) {
        return error;
    }

    /* Note that we are attached. */
    pd->extension = ext;
    ext->attached = 1;

    return 0;
}

