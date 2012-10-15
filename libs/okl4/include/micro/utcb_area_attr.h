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

#ifndef __OKL4__UTCB_AREA_ATTR_H__
#define __OKL4__UTCB_AREA_ATTR_H__

/**
 *  @file
 *
 *  The utcb_area_attr.h Header.
 *
 *  The utcb_area_attr.h header provides the functionality required for making
 *  use of UTCB area attributes, which are used to initialise UTCB areas.
 *
 *  The UTCB area is a region of virtual memory, defined for each address
 *  space, that contains all the UTCBs for threads contained in that space.
 *  The size of the UTCB area in a given address space determines how many
 *  maximum threads that address space may support.  The UTCB area also
 *  acts as an allocator for UTCB items.
 *
 */

/**
 *  The okl4_utcb_area_attr struct is used to represent the UTCB
 *  area attribute.  This structure is used to initialize UTCB areas.
 *
 *  There are two steps required to construct a UTCB area attribute:
 *
 *  @li The attribute must be initialized (okl4_utcb_area_attr_init()), and
 *
 *  @li The UTCB area's virtual memory region must be defined
 *  (okl4_utcb_area_attr_setrange()).
 *
 *  Both of these steps must be performed on the UTCB area attribute before
 *  it can be used to initialize UTCB areas.
 *
 */
struct okl4_utcb_area_attr {
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    okl4_virtmem_item_t utcb_range;
};

/**
 *  The okl4_utcb_area_attr_init() function is used to initialize the UTCB
 *  area attribute @a attr to default values.  It must be invoked on the
 *  attribute before it may be used for UTCB area initialization.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to initialize.  Its contents will contain default
 *    values after the function returns.
 *
 */
INLINE void okl4_utcb_area_attr_init(okl4_utcb_area_attr_t *attr);

/**
 *  The okl4_utcb_area_attr_setrange() function is used to encode the UTCB
 *  area attribute @a attr with a virtual memory region. This defines the
 *  location and size of the UTCB area that is initialized with this
 *  attribute.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to encode.  Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param base
 *    The base virtual address of the UTCB area.
 *
 *  @param size
 *    The size of the UTCB area.
 *
 */
INLINE void okl4_utcb_area_attr_setrange(okl4_utcb_area_attr_t *attr,
        okl4_word_t base, okl4_word_t size);

/*
 * Inlined methods
 */

INLINE void
okl4_utcb_area_attr_init(okl4_utcb_area_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_UTCB_AREA_ATTR);
    okl4_range_item_setsize(&attr->utcb_range, 0);
}

INLINE void
okl4_utcb_area_attr_setrange(okl4_utcb_area_attr_t *attr,
        okl4_word_t base, okl4_word_t size)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_UTCB_AREA_ATTR);

    okl4_range_item_setrange(&attr->utcb_range, base, size);
}


#endif /* ! __OKL4__UTCB_AREA_ATTR_H__ */
