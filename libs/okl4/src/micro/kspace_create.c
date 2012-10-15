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

#include <okl4/kspace.h>

int
okl4_kspace_create(okl4_kspace_t *kspace, okl4_kspace_attr_t *attr)
{
    okl4_word_t result;
#if !defined(NO_UTCB_RELOCATE)
    okl4_word_t utcb_base, utcb_size;
#endif
    L4_Fpage_t utcb_area;

    assert(kspace != NULL);
    assert(attr != NULL);

    /* If the attr == OKL4_WEAVED_OBJECT, we assume that the kspace object has
     * been weaved in and hence, we do not need to re-initialize it.
     */
    if (attr != OKL4_WEAVED_OBJECT) {
        kspace->id = attr->id;
        kspace->kclist = attr->kclist;
        kspace->utcb_area = attr->utcb_area;
        kspace->kthread_list = NULL;
        kspace->next = NULL;
        kspace->privileged = attr->privileged;
    }

    /*
     * Now create the actual kernel space using SpaceControl syscall
     * Create a Fpage for a utcb and then pass it along
     */
#if !defined(NO_UTCB_RELOCATE)
    utcb_base = kspace->utcb_area->utcb_memory.base;
    utcb_size = kspace->utcb_area->utcb_memory.size;

    /* Fpages require that they are aligned to their size. */
    assert(utcb_base % utcb_size == 0);

    utcb_area = L4_Fpage(utcb_base, utcb_size);
#else
    utcb_area = L4_Nilpage;
#endif

    result = L4_SpaceControl(kspace->id,
            L4_SpaceCtrl_new
                | (attr->privileged ? L4_SpaceCtrl_kresources_accessible : 0),
            kspace->kclist->id, utcb_area, 0, NULL);
    if (result != 1) {
        return _okl4_convert_kernel_errno(L4_ErrorCode());
    }

    /* Add this kspace into the kclist's list of kspaces */
    kspace->next = kspace->kclist->kspace_list;
    kspace->kclist->kspace_list = kspace;

    return OKL4_OK;
}
