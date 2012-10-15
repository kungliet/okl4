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
#ifndef __L4__MAP_OLD_H__
#define __L4__MAP_OLD_H__

/*
 * MapItem type from MapControl
 */
typedef union {
    L4_Word_t raw;
    struct {
        L4_BITFIELD4( L4_Word_t,
                rwx     : 4,
                size    : 6,
                __res   : WORD_T_BIT - 14,
                ref     : 4);
    } X;
} L4_OldMapItem_t;

/*
 * Physical descriptor from MapControl item
 */
typedef union {
    L4_Word_t raw;
    struct {
        L4_BITFIELD2(L4_Word_t, attr:6, base: WORD_T_BIT - 6);
    } X;
} L4_PhysDesc_t;

L4_INLINE L4_PhysDesc_t
L4_PhysDesc(L4_Word64_t PhysAddr, L4_Word_t Attribute)
{
    L4_PhysDesc_t phys;

    phys.raw = 0;

    phys.X.base = PhysAddr >> 10;
    phys.X.attr = Attribute;
    return phys;
}

typedef union {
    L4_Word_t raw;
    struct {
        L4_BITFIELD4(L4_Word_t,
                rwx         : 4,
                page_size   : 6,
                ref         : 4,
                __res       : WORD_T_BIT - 14);
    } X;
} L4_QueryDesc_t;

typedef struct {
    L4_SegmentDesc_t    seg;
    L4_QueryDesc_t      query;
    L4_VirtualDesc_t    virt;
} L4_QueryItem_t;

L4_INLINE void
L4_Map_SetSegmentId(L4_MapItem_t *m, L4_Word_t x)
{
    m->seg.X.segment = x;
}

L4_INLINE void
L4_Map_SetOffset(L4_MapItem_t *m, L4_Word_t x)
{
    m->seg.X.offset = x;
}

L4_INLINE void
L4_Map_SetPageSize(L4_MapItem_t *m, L4_Word_t x)
{
    m->size.X.page_size = x;
}

L4_INLINE void
L4_Map_SetRwx(L4_MapItem_t *m, L4_Word_t x)
{
    m->size.X.rwx = x;
}

L4_INLINE void
L4_Map_SetVbase(L4_MapItem_t *m, L4_Word_t x)
{
    m->virt.X.vbase = x;
}

L4_INLINE void
L4_Map_SetAttr(L4_MapItem_t *m, L4_Word_t x)
{
    m->virt.X.attr = x;
}

/*
 * MapControl controls
 */
#define L4_MapCtrl_Query        ((L4_Word_t)1 << (L4_BITS_PER_WORD - 2))
#define L4_MapCtrl_Modify       ((L4_Word_t)1 << (L4_BITS_PER_WORD - 1))
#if defined(ARM_SHARED_DOMAINS)
#define L4_MapCtrl_MapWindow    ((L4_Word_t)1 << (L4_BITS_PER_WORD - 3))
#endif

#endif /* !__L4__MAP_OLD_H__ */
