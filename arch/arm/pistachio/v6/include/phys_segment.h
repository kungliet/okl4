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

#ifndef __ARCH__ARM__V6__PHYS_SEGMENT_H__
#define __ARCH__ARM__V6__PHYS_SEGMENT_H__

#include <cpu/phys_segment.h>

#ifdef __USE_DEFAULT_V6_CACHE_ATTRIBS
#undef __USE_DEFAULT_V6_CACHE_ATTRIBS

enum allowed_attrib_e {
    allowed_uncached        = 0x01,
    allowed_strong          = 0x02,
    allowed_iomemory        = 0x04,
    allowed_cohearent       = 0x08,
    allowed_writeback       = 0x10,
    allowed_writethrough    = 0x20,
    allowed_custom          = 0x40,
};

/**
 * Convert a memory attribute into a set of flags that can then be checked
 * against a segment attribute permissions. Any flags set which are not set in
 * the segment indicates the requested attributes are invalid.
 */
INLINE word_t to_physattrib(memattrib_e attr)
{
    word_t res = 0UL;

    switch (attr) {
    case strong:
        return (int)allowed_strong;
    case iomemory:
        return (int)allowed_iomemory;
    case iomemory_shared:
        return (int)allowed_iomemory | (int)allowed_cohearent;
    case uncached:
    case uncached_shared:
        res = (int)allowed_uncached; break;
    case writeback:
    case writeback_shared:
    case writeback_alloc:
    case writeback_alloc_shared:
        res = (int)allowed_writeback; break;
    case writethrough:
    case writethrough_shared:
        res = (int)allowed_writethrough; break;
    default:
        if (((int)attr & CACHE_ATTRIB_CUSTOM_MASK) == CACHE_ATTRIB_CUSTOM_L1L2) {
            res = (int)allowed_custom; break;
        }
        return (~0UL);
    }

    /* check shareable attributes */
    if ((int)attr & CACHE_ATTRIB_SHARED) {
        res |= (int)allowed_cohearent;
    }

    return res;
}

#endif

#endif /*__ARCH__ARM__V6__PHYS_SEGMENT_H__*/
