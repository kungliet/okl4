/*
 * Copyright (c) 2005, National ICT Australai
 */
/*
 * Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
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
/*
 * Description:   Interfaces for mappings in map control
 */
#ifndef __L4__MAP_H__
#define __L4__MAP_H__

#include <l4/types.h>
#include <l4/map_types.h>
#include <l4/arch/config.h>

#define L4_MAX_MAP_ITEMS    (__L4_NUM_MRS / 3)

/*
 * MapItem and QueryItem
 */

typedef union {
    L4_Word_t raw;
    virtual_descriptor_t X;
} L4_VirtualDesc_t;

typedef union {
    L4_Word_t raw;
    size_descriptor_t X;
} L4_SizeDesc_t;

typedef union {
    L4_Word_t raw;
    segment_descriptor_t X;
} L4_SegmentDesc_t;

typedef struct {
    L4_SegmentDesc_t    seg;
    L4_SizeDesc_t       size;
    L4_VirtualDesc_t    virt;
} L4_MapItem_t;

L4_INLINE void
L4_MapItem_Map(L4_MapItem_t *m, L4_Word_t segment_id, L4_Word_t offset,
               L4_Word_t vaddr, L4_Word_t page_size, L4_Word_t attr,
               L4_Word_t rwx)
{
    L4_SegmentDesc_t    seg;
    L4_SizeDesc_t       size;
    L4_VirtualDesc_t    virt;

    seg.X.segment = segment_id;
    seg.X.offset = offset >> __L4_MAP_SHIFT;

    size.X.num_pages = 1;
    size.X.valid = 1;
    size.X.page_size = page_size;
    size.X.rwx = rwx;

    virt.raw = 0UL;
    virt.X.vbase = vaddr >> __L4_MAP_SHIFT;
    virt.X.attr = attr;

    m->seg = seg;
    m->size = size;
    m->virt = virt;
}

L4_INLINE void
L4_MapItem_MapReplace(L4_MapItem_t *m, L4_Word_t segment_id, L4_Word_t offset,
               L4_Word_t vaddr, L4_Word_t page_size, L4_Word_t attr,
               L4_Word_t rwx)
{
    L4_MapItem_Map(m, segment_id, offset, vaddr, page_size, attr, rwx);
    m->virt.X.replace = 1;
}

L4_INLINE void
L4_MapItem_Unmap(L4_MapItem_t *m, L4_Word_t vaddr, L4_Word_t page_size)
{
    L4_SegmentDesc_t    seg;
    L4_SizeDesc_t       size;
    L4_VirtualDesc_t    virt;

    seg.raw = 0;
    size.raw = 0;
    virt.raw = 0;

    size.X.valid = 0;
    size.X.num_pages = 1;
    size.X.page_size = page_size;
    virt.X.vbase = vaddr >> __L4_MAP_SHIFT;

    m->seg = seg;
    m->size = size;
    m->virt = virt;
}

L4_INLINE void
L4_MapItem_SparseUnmap(L4_MapItem_t *m, L4_Word_t vaddr, L4_Word_t page_size)
{
    L4_MapItem_Unmap(m, vaddr, page_size);
    m->virt.X.sparse = 1;
}

L4_INLINE void
L4_MapItem_SetMultiplePages(L4_MapItem_t *m, L4_Word_t num_pages)
{
    m->size.X.num_pages = num_pages;
}

L4_INLINE L4_Bool_t
L4_PageRightsSupported(L4_Word_t rwx)
{
    return ((1 << rwx) & __L4_VALID_HW_PAGE_PERMS);
}

#include "map_old.h"

#endif /* !__L4__MAP_H__ */
