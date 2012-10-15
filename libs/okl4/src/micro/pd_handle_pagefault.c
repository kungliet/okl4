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

#include <l4/types.h>

#include <okl4/types.h>
#include <okl4/pd.h>
#include <okl4/kspace.h>
#include <okl4/memsec.h>
#include <okl4/zone.h>
#include <okl4/extension.h>

#include "pd_helpers.h"

/**
 * Map the given address into the given protection domain.
 */
int
okl4_pd_handle_pagefault(okl4_pd_t *pd, okl4_kspaceid_t space,
        okl4_word_t vaddr, okl4_word_t access_type)
{
    int error;
    _okl4_mem_container_t *container;
    _okl4_mem_container_type_t type;
    okl4_kspace_map_attr_t attr;
    okl4_virtmem_item_t range;
    okl4_physmem_item_t phys;
    okl4_word_t dest_vaddr;
    okl4_word_t perms;
    okl4_word_t attach_perms;
    okl4_word_t attributes;
    okl4_word_t page_size;
    int extension_space_fault;
    int map_into_base;
    int map_into_ext;

    /* Get the container the address falls into. */
    container = _okl4_get_container_from_vaddr(
            pd->mem_container_list, vaddr, &type, &attach_perms);
    if (container == NULL) {
        return OKL4_UNMAPPED;
    }

    /* Get the mapping from our memsec or zone */
    switch (type) {
        case _OKL4_TYPE_MEMSECTION:
            error = okl4_memsec_access((okl4_memsec_t *)container, vaddr, &phys,
                    &dest_vaddr, &page_size, &perms, &attributes);
            break;
        case _OKL4_TYPE_ZONE:
            error = _okl4_zone_access((okl4_zone_t *)container, vaddr, &phys,
                    &dest_vaddr, &page_size, &perms, &attributes);
            break;
        case _OKL4_TYPE_EXTENSION:
            assert(pd->extension != NULL);
            error = _okl4_extension_access((okl4_extension_t *)container, vaddr,
                    &phys, &dest_vaddr, &page_size, &perms, &attributes);
            break;
        default:
            while (1) {
                assert(!"Bad mem_container type");
            }
    }

    /* If the mapping couldn't be performed, abort. */
    if (error) {
        return error;
    }

    /* Did the fault occur in an extension space? */
    extension_space_fault =
            (pd->extension != NULL
                    && L4_IsSpaceEqual(pd->extension->kspace->id, space));

    /* If the fault occurred in an extension area, but not in the extension
     * space, return an error. */
    if (!extension_space_fault && type == _OKL4_TYPE_EXTENSION) {
        return OKL4_UNMAPPED;
    }

    /* Setup the mapping. */
    okl4_range_item_setrange(&range, dest_vaddr, phys.range.size);
    okl4_kspace_map_attr_init(&attr);
    okl4_kspace_map_attr_setperms(&attr, perms & attach_perms);
    okl4_kspace_map_attr_setattributes(&attr, attributes);
    okl4_kspace_map_attr_setpagesize(&attr, page_size);
    okl4_kspace_map_attr_settarget(&attr, &phys);
    okl4_kspace_map_attr_setrange(&attr, &range);

    /* Determine if we should map into the base space. */
    map_into_base = (type != _OKL4_TYPE_EXTENSION);

    /* Determine if we should map into the extension space. */
#ifndef ARM_SHARED_DOMAINS
    /* We always map into the extension space. */
    map_into_ext = 1; /*lint -e774 */
#else
    /* We map into the extension space only if there is no window into the base
     * space. (i.e., if this is a fault on a extension mapping). */
    map_into_ext = (type == _OKL4_TYPE_EXTENSION);
#endif

    /* Map into base space if required. */
    if (map_into_base) {
        error = okl4_kspace_map(pd->kspace, &attr);
        if (error) {
            return error;
        }
    }

    /* If we have an extension space and we should map it into the
     * extension space, do so. */
    if (pd->extension != NULL && map_into_ext) {
        error = okl4_kspace_map(pd->extension->kspace, &attr);
        if (error) {
            return error;
        }
    }

    return OKL4_OK;
}

