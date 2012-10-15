/*
 * Copyright (c) 2001-2004, Karlsruhe University
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
 * Description:   Interfaces for handling address spaces/mappings
 */
#ifndef __L4__SPACE_COMPAT_H__
#define __L4__SPACE_COMPAT_H__

#include <l4/types.h>
#include <l4/map.h>
#include <l4/kdebug.h>
#include <l4/wbtest.h>
#include <l4/arch/syscalls.h>


L4_INLINE L4_Word_t
L4_ReadMemRegions(L4_SpaceId_t s, L4_Word_t n, L4_MapItem_t *map_items,
                                                L4_QueryItem_t *query)
{
    int i;
    for (i = 0; i < n && i < L4_MAX_MAP_ITEMS; i++) {
        word_t f_phys, f_size, f_attr, f_rwx;
        L4_Word_t r;
        r = L4_WBT_GetMapping(s, map_items[i].virt.X.vbase << 10, 0,
                              &f_phys, &f_size, &f_attr, &f_rwx);
        if (query) {
            query[i].seg.X.segment = 0;
            query[i].seg.X.offset = f_phys >> 10;

            query[i].query.X.rwx = f_rwx;
            query[i].query.X.page_size = r ? f_size : 0;
            query[i].query.X.ref = 0; // XXX

            query[i].virt.X.attr = f_attr;
            query[i].virt.X.vbase = map_items[i].virt.X.vbase;
        }
    }
    return 1;
}

L4_INLINE L4_Word_t
L4_ReadMemRegion(L4_SpaceId_t s, L4_MapItem_t map_item, L4_QueryItem_t *query)
{
    return L4_ReadMemRegions(s, 1, &map_item, query);
}

/*
 * Derived functions
 */

L4_INLINE L4_Word_t
L4_MapFpage(L4_SpaceId_t s, L4_Fpage_t f, L4_PhysDesc_t p)
{
    L4_MapItem_t map_item;

    L4_MapItem_Map(&map_item, 0, p.X.base << __L4_MAP_SHIFT, f.X.b << __L4_MAP_SHIFT,
                                                        f.X.s, p.X.attr, f.X.rwx);

    if (f.X.rwx == 0) {
        map_item.size.X.valid = 0;
    }

    return L4_ProcessMapItem(s, map_item);
}

L4_INLINE L4_Word_t
L4_MapFpages(L4_SpaceId_t s,
             L4_Word_t n, L4_Fpage_t *f, L4_PhysDesc_t *p)
{
    L4_Word_t i;
    // XXX: 30
    L4_MapItem_t map_items[L4_MAX_MAP_ITEMS];

    for (i = 0; i < n && i < L4_MAX_MAP_ITEMS; i++) {
        L4_MapItem_Map(&map_items[i], 0, p[i].X.base << __L4_MAP_SHIFT,
                    f[i].X.b << __L4_MAP_SHIFT, f[i].X.s, p[i].X.attr, f[i].X.rwx);
        if (f[i].X.rwx == 0) {
            map_items[i].size.X.valid = 0;
        }
    }
    return L4_ProcessMapItems(s, n, map_items);
}

L4_INLINE L4_Word_t
L4_UnmapFpage(L4_SpaceId_t s, L4_Fpage_t f)
{
    L4_MapItem_t map_item;

    L4_MapItem_Unmap(&map_item, f.X.b << __L4_MAP_SHIFT, f.X.s);

    return L4_ProcessMapItem(s, map_item);
}

L4_INLINE L4_Word_t
L4_UnmapFpages(L4_SpaceId_t s, L4_Word_t n, L4_Fpage_t *f)
{
    L4_Word_t i;
    L4_MapItem_t map_items[L4_MAX_MAP_ITEMS];

    for (i = 0; i < n && i < L4_MAX_MAP_ITEMS; i++) {
        L4_MapItem_Unmap(&map_items[i], f[i].X.b << __L4_MAP_SHIFT, f[i].X.s);
    }
    return L4_ProcessMapItems(s, n, map_items);
}

#if defined(__l4_cplusplus)
L4_INLINE L4_Fpage_t
L4_Unmap(L4_SpaceId_t s, L4_Fpage_t f)
{
    return L4_UnmapFpage(s, f);
}

L4_INLINE L4_Word_t
L4_Unmap(L4_SpaceId_td s, L4_Word_t n,
         L4_Fpage_t *fpages, L4_PhysDesc_t *descs)
{
    return L4_UnmapFpages(s, n, fpages, descs);
}
#endif

L4_INLINE L4_Word_t
L4_ReadFpage(L4_SpaceId_t s, L4_Fpage_t f, L4_PhysDesc_t *p, L4_OldMapItem_t *m)
{
    L4_Word_t r;
    L4_MapItem_t map_item;
    L4_QueryItem_t query_item;

    L4_MapItem_Unmap(&map_item, f.X.b << __L4_MAP_SHIFT, f.X.s);

    r = L4_ReadMemRegion(s, map_item, &query_item);
    if (m) {
        m->X.rwx = query_item.query.X.rwx;
        m->X.size = query_item.query.X.page_size;
        m->X.ref = query_item.query.X.ref;
    }
    if (p) {
        p->X.attr = query_item.virt.X.attr;
        p->X.base = query_item.seg.X.offset;
    }
    return r;
}

L4_INLINE L4_Word_t
L4_ReadFpages(L4_SpaceId_t s, L4_Word_t n, L4_Fpage_t *f, L4_PhysDesc_t *p,
                                                L4_OldMapItem_t *m)
{
    L4_Word_t i;
    L4_Word_t r;
    L4_QueryItem_t query[L4_MAX_MAP_ITEMS];
    L4_MapItem_t map_items[L4_MAX_MAP_ITEMS];

    for (i = 0; i < n && i < L4_MAX_MAP_ITEMS; i++) {
        L4_MapItem_Unmap(&map_items[i], f[i].X.b << __L4_MAP_SHIFT, f[i].X.s);
    }
    r = L4_ReadMemRegions(s, n, map_items, query);

    for (i = 0; i < n && i < L4_MAX_MAP_ITEMS; i++) {
        if (m) {
            m[i].raw = 0UL;
            m[i].X.rwx = query[i].query.X.rwx;
            m[i].X.size = query[i].query.X.page_size;
            m[i].X.ref = query[i].query.X.ref;
        }
        if (p) {
            p[i].raw = 0UL;
            p[i].X.attr = query[i].virt.X.attr;
            p[i].X.base = query[i].seg.X.offset;
        }
    }
    return r;
}

L4_INLINE L4_Bool_t
L4_WasWritten(L4_Fpage_t f)
{
    return (f.raw & L4_Writable) != 0;
}

L4_INLINE L4_Bool_t
L4_WasReferenced(L4_Fpage_t f)
{
    return (f.raw & L4_Readable) != 0;
}

L4_INLINE L4_Bool_t
L4_WaseXecuted(L4_Fpage_t f)
{
    return (f.raw & L4_eXecutable) != 0;
}

#endif /* !__L4__SPACE_COMPAT_H__ */
