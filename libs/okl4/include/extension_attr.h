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

#ifndef __OKL4__EXTENSION_ATTR_H__
#define __OKL4__EXTENSION_ATTR_H__

#include <assert.h>

#include <l4/types.h>

#include <okl4/types.h>

/**
 *  @file
 *
 *  The extension_attr.h Header.
 *
 *  The extension_attr.h header provides the functionality required for 
 *  the PD extension attribute used to initialize PD
 *  extensions.
 *
 */

/**
 *  The okl4_extension_attr structure is used to represent the
 *  extension attribute which may be subsequently used to create extensions.
 *
 *  The following steps need to be performed on an extension attribute
 *  before it may be used to create extensions:
 *
 *  @li The attribute must be initialized.
 *
 *  @li The virtual memory region covered by the extension must be set.
 *
 *  @li The protection domain to which the extension belongs must be set.
 *
 *  @li Creating an extension also creates a new kspace in the
 *  process. Thus, the kspace id allocator to be used for this purpose must also 
 *  be set. 
 *
 */
struct okl4_extension_attr {
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif

    /* Virtual address range this extension is responsable for. */
    okl4_virtmem_item_t range;

    /* Base protection domain this extension extends. */
    okl4_pd_t *base_pd;

    /* Pool of kspace identifiers. */
    okl4_kspaceid_pool_t *kspaceid_pool;
};

/**
 *  The okl4_extension_attr_init() function is used to initialize the
 *  extension attribute @a attr to default values. It must be invoked on
 *  the attribute before it is used for zone creation.
 *
 */
INLINE void okl4_extension_attr_init(okl4_extension_attr_t *attr);

/**
 *  The okl4_extension_attr_setrange() function is used to encode a virtual
 *  memory region specified by the @a range arguments in the extension 
 *  attribute specified by the @a attr argument. This specifies the virtual 
 *  memory region covered by the extension when it is subsequently attached 
 *  to a PD. Note that the memory section attached to an extension must be
 *  entirely contained within the virtual memory region of the extension.
 *
 *  Note that an extension cannot be attached to a particular PD if 
 *  its virtual address range overlaps with any other 
 *  memory sections, zones, or extensions already attached to that PD.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be encoded. 
 *
 *  @param range
 *    The virtual memory region of the extension.
 *
 */
INLINE void okl4_extension_attr_setrange(okl4_extension_attr_t *attr,
        okl4_virtmem_item_t range);

/**
 *  The okl4_extension_attr_setbasepd() function is used to encode the
 *  protection domain @a basepd to which the extension will belong.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be encoded. 
 *
 *  @param basepd
 *    The PD to which the extension will belong.
 *
 */
INLINE void okl4_extension_attr_setbasepd(okl4_extension_attr_t *attr,
        okl4_pd_t *basepd);

/**
 *  The okl4_extension_attr_setkspaceidpool() function is used to encode a
 *  kspace id pool specified by the @a kspaceid_pool argument in the extension 
 *  attribute specified by the @a attr argument.
 *  As creating an extension requires creating a new kspace, and
 *  creating a kspace requires allocating from a pool of available kspace
 *  ids, the kspace id allocator to allocate from must be specified when
 *  an extension is created.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be encoded. 
 *
 *  @param kspaceid_pool
 *    The kspace id allocator from which the kspace id is to be allocated.
 *
 */
INLINE void okl4_extension_attr_setkspaceidpool(okl4_extension_attr_t *attr,
        okl4_kspaceid_pool_t *kspaceid_pool);

INLINE void
okl4_extension_attr_init(okl4_extension_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_EXTENSION_ATTR);
    okl4_range_item_setsize(&attr->range, 0);
    attr->base_pd = NULL;
    attr->kspaceid_pool = NULL;
}

INLINE void
okl4_extension_attr_setrange(okl4_extension_attr_t *attr,
        okl4_virtmem_item_t range)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_EXTENSION_ATTR);

    attr->range = range;
}

INLINE void
okl4_extension_attr_setbasepd(okl4_extension_attr_t *attr, okl4_pd_t *basepd)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_EXTENSION_ATTR);

    attr->base_pd = basepd;
}

INLINE void
okl4_extension_attr_setkspaceidpool(okl4_extension_attr_t *attr,
        okl4_kspaceid_pool_t *kspaceid_pool)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_EXTENSION_ATTR);

    attr->kspaceid_pool = kspaceid_pool;
}


#endif /* !__OKL4__EXTENSION_ATTR_H__ */
