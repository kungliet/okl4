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

#if defined(ARM_SHARED_DOMAINS)

#include <l4/map.h>
#include <l4/arch/ver/space_resources.h>
#include <l4/space.h>

#include <okl4/kspace.h>
#include <okl4/errno.h>
#include <okl4/types.h>


int
_okl4_kspace_domain_share(okl4_kspaceid_t client,
        okl4_kspaceid_t share, int manager)
{
    okl4_word_t error;
    okl4_word_t op = L4_SPACE_RESOURCES_WINDOW_GRANT;
    L4_ClistId_t null_clist = {0};

    if (manager) {
        op = L4_SPACE_RESOURCES_WINDOW_MANAGE;
    }

    L4_LoadMR(0, share.raw);
    error = L4_SpaceControl(client, L4_SpaceCtrl_resources,
            null_clist, L4_Nilpage, op, NULL);
    if (error != 1) {
        return _okl4_convert_kernel_errno(L4_ErrorCode());
    }

    return OKL4_OK;
}

void
_okl4_kspace_domain_unshare(okl4_kspaceid_t client, okl4_kspaceid_t share)
{
    okl4_word_t error;
    L4_ClistId_t null_clist = {0};

    L4_LoadMR(0, share.raw);
    error = L4_SpaceControl(client, L4_SpaceCtrl_resources,
            null_clist, L4_Nilpage,
            L4_SPACE_RESOURCES_WINDOW_REVOKE, NULL);
    assert(error == 1);
}

int
_okl4_kspace_window_map(okl4_kspaceid_t client, okl4_kspaceid_t share,
        okl4_range_item_t window)
{
    okl4_word_t error;
    L4_Fpage_t fpage;

    /* Ensure window alignment is correct. */
    assert(window.base % _OKL4_WINDOW_ALIGNMENT == 0);
    assert(window.size % _OKL4_WINDOW_ALIGNMENT == 0);

    fpage = L4_Fpage(window.base, window.size);
    (void)L4_Set_Rights(&fpage, L4_FullyAccessible);
    L4_LoadMR(0, share.raw);
    L4_LoadMR(1, fpage.raw);
    error = L4_MapControl(client, L4_MapCtrl_MapWindow);
    if (error != 1) {
        return _okl4_convert_kernel_errno(L4_ErrorCode());
    }

    return OKL4_OK;
}

int
_okl4_kspace_window_unmap(okl4_kspaceid_t client, okl4_kspaceid_t share,
        okl4_range_item_t window)
{
    okl4_word_t error;
    L4_Fpage_t fpage = L4_Fpage(window.base, window.size);

    (void)L4_Set_Rights(&fpage, L4_NoAccess);
    L4_LoadMR(0, share.raw);
    L4_LoadMR(1, fpage.raw);
    error = L4_MapControl(client, L4_MapCtrl_MapWindow);
    if (error != 1) {
        return _okl4_convert_kernel_errno(L4_ErrorCode());
    }

    return OKL4_OK;
}

/*
 * Share a domain between the two spaces, and add a window.
 */
int
_okl4_kspace_share_domain_add_window(okl4_kspaceid_t client,
        okl4_kspaceid_t owner, int manager, okl4_virtmem_item_t window)
{
    int error;

    /* Share the zone space. */
    error = _okl4_kspace_domain_share(client, owner, manager);
    if (error) {
        return error;
    }

    /* Create a window between the two spaces. */
    error = _okl4_kspace_window_map(client, owner, window);
    if (error) {
        return error;
    }

    return OKL4_OK;
}

#endif

