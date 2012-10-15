/*
 * Copyright (c) 2006, National ICT Australia
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
 * Description:   ARMv6/v7 page table structures
 * Author: Carl van Schaik, June 2006
 */
#ifndef __ARCH__ARMV6__MEMATTRIB_H__
#define __ARCH__ARMV6__MEMATTRIB_H__

#include <cache.h>

#if !defined(CONFIG_TEX_REMAPPING)

#include <l4/arch/cache_attribs.h>

/**
 * Encoding format of memattrib_e:
 *  B   - bit 0
 *  C   - bit 1
 *  TEX - bit 4..2
 *        bit 6..5 (zero - reserved)
 *  S   - bit 7
 *  NS  - bit 8 (non-secure - trustzone only)
 */

enum memattrib_e {

    strong              = CACHE_ATTRIB_STRONG,
    writethrough        = CACHE_ATTRIB_WRITETHROUGH,
    writeback           = CACHE_ATTRIB_WRITEBACK,
    writeback_alloc     = CACHE_ATTRIB_WRITEBACK_ALLOC,
    uncached            = CACHE_ATTRIB_UNCACHED,
    iomemory            = CACHE_ATTRIB_IOMEMORY,
    iomemory_shared     = CACHE_ATTRIB_IOMEMORY_SHARED,

    writethrough_shared = CACHE_ATTRIB_WRITETHROUGH | CACHE_ATTRIB_SHARED,
    writeback_shared    = CACHE_ATTRIB_WRITEBACK    | CACHE_ATTRIB_SHARED,
    writeback_alloc_shared = CACHE_ATTRIB_WRITEBACK_ALLOC | CACHE_ATTRIB_SHARED,
    uncached_shared     = CACHE_ATTRIB_UNCACHED     | CACHE_ATTRIB_SHARED,

    /*
     * Other attributes, use CACHE_ATTRIB_CUSTOM(L1, L2)
     * eg:
     *  NC_NC           = CACHE_ATTRIB_CUSTOM(CACHE_ATTRIB_NC, CACHE_ATTRIB_NC)
     *  WT_WB           = CACHE_ATTRIB_CUSTOM(CACHE_ATTRIB_WT, CACHE_ATTRIB_WB)
     *  WT_WB_shared    = CACHE_ATTRIB_CUSTOM(CACHE_ATTRIB_WT, CACHE_ATTRIB_WB) | CACHE_ATTRIB_SHARED
     */

#if defined(CONFIG_SMP)
 #if defined(CONFIG_DEFAULT_CACHE_ATTR_WB)
    l4default           = writeback_shared,
 #else
    l4default           = writethrough_shared, /* NB: this will affect performance */
 #endif
#else
 #if defined(CONFIG_DEFAULT_CACHE_ATTR_WB)
    l4default           = writeback,
 #else
    l4default           = writethrough, /* NB: this will affect performance */
 #endif
#endif
};

#else /* CONFIG_TEX_REMAPPING */
#error TEX remapping not implemented
#endif

#endif /* !__ARCH__ARMV6__MEMATTRIB_H__ */
