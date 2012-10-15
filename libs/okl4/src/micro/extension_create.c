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
#include <l4/message.h>

#if defined(ARM_SHARED_DOMAINS)
#include <l4/arch/ver/space_resources.h>
#endif

#include <okl4/memsec.h>
#include <okl4/extension.h>

#include "pd_helpers.h"

/*
 * Create a new extension based on the provided attributes object.
 */
int
okl4_extension_create(okl4_extension_t *extension, okl4_extension_attr_t *attr)
{
    int error;
    okl4_kspace_attr_t kspace_attr;
#if defined(ARM_SHARED_DOMAINS)
    okl4_word_t success;
    L4_ClistId_t null_clist = {0};
#endif

    assert(extension != NULL);
    assert(attr != NULL);
    assert(attr->base_pd != NULL);
    assert(attr->kspaceid_pool != NULL);

    /* Ensure that the extension is aligned correctly. */
    assert(attr->range.base % OKL4_EXTENSION_ALIGNMENT == 0);
    assert(attr->range.size % OKL4_EXTENSION_ALIGNMENT == 0);

    /* Setup the extension struct. */
    extension->kspace = &extension->_init.kspace_mem;
    extension->attached = 0;
    extension->mem_container_list = NULL;

    /* Copy attributes over to the extension structure. */
    extension->base_pd = attr->base_pd;
    extension->_init.kspaceid_pool = attr->kspaceid_pool;
    extension->super.type = _OKL4_TYPE_EXTENSION;
    extension->super.range = attr->range;

    /* Ensure that the PD is privileged. */
    if (!attr->base_pd->kspace->privileged) {
        return OKL4_INVALID_ARGUMENT;
    }

    /* Allocate a kspace ID. */
    error = okl4_kspaceid_allocany(attr->kspaceid_pool,
            &extension->_init.kspace_id);
    if (error) {
        return error;
    }

    /* Create a new space. */
    okl4_kspace_attr_init(&kspace_attr);
    okl4_kspace_attr_setkclist(&kspace_attr, attr->base_pd->kspace->kclist);
    okl4_kspace_attr_setutcbarea(&kspace_attr, attr->base_pd->kspace->utcb_area);
    okl4_kspace_attr_setid(&kspace_attr, extension->_init.kspace_id);
    okl4_kspace_attr_setprivileged(&kspace_attr, 1);
    error = okl4_kspace_create(extension->kspace, &kspace_attr);
    if (error) {
        okl4_kspaceid_free(attr->kspaceid_pool,
                extension->_init.kspace_id);
        return error;
    }

#if defined(ARM_SHARED_DOMAINS)
    /* Share the base space domain with the extension space. */
    L4_LoadMR(0, extension->base_pd->kspace->id.raw);
    success = L4_SpaceControl(extension->kspace->id,
            L4_SpaceCtrl_resources, null_clist, L4_Nilpage,
            L4_SPACE_RESOURCES_WINDOW_GRANT, NULL);
    assert(success);
#endif

    return OKL4_OK;
}

