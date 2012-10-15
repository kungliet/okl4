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

#ifndef __OKL4__PHYSMEM_OBJECT_H__
#define __OKL4__PHYSMEM_OBJECT_H__

#include <okl4/types.h>
#include <okl4/errno.h>
#include <okl4/range.h>

/**
 *  @file
 *
 *  The physmem_item.h Header.
 *
 *  The physmem_item.h header provides the functionality required for
 *  using the physical memory allocators, namely the @a physseg
 *  allocator  and physpool allocator.
 *  This header file defines the @a physmem item object and its operations,
 *  which are used to encode allocation parameters to the physmem
 *  allocators.  This header also defines the @a physmem allocator
 *  attribute object and its operations, which are used to initialize the
 *  physmem allocators.
 *
 *
 */

/**
 *  The okl4_physmem_item structure is used to represent the @a
 *  physmem item, a region of physical memory. This represents the kernels view
 *  of physical memory at the user level.
 *  The physmem item is used to encode allocation and deallocation
 *  parameters, as well as allocation results, from the physmem allocators.
 *
 */
struct okl4_physmem_item {
    okl4_word_t segment_id;
    okl4_word_t paddr;
    okl4_range_item_t range;
};

/* Pre-initialisation accessor methods */

/**
 *  The okl4_physmem_item_setrange() function is used to encode a physmem
 *  item to represent a region of physical memory with a @a offset address
 *  and @a size.
 *
 *  @param physobj
 *    The physmem item to encode. Its contents will contain the encoding
 *    on function return.
 *
 *  @param offset
 *    The offset address of the physical memory region.
 *
 *  @param size
 *    The size of the physical memory region.
 *
 */
INLINE void okl4_physmem_item_setrange(okl4_physmem_item_t * physobj,
        okl4_word_t offset, okl4_word_t size);

/**
 *  The okl4_physmem_item_setsize() function is used to encode a physmem
 *  item to represent a region of physical memory of a size specified by 
 *  the @a size argument.  The base value of the interval is unspecified.
 *
 *  This function expects the following arguments:
 *
 *  @param physobj
 *    The physmem item to encode. Its contents will contain the encoding
 *    on function return.
 *
 *  @param length
 *    The size of the physical memory region.
 *
 */
INLINE void okl4_physmem_item_setsize(okl4_physmem_item_t * physobj,
        okl4_word_t size);

/**
 *  The okl4_physmem_item_setsegment() function is used to set the physical
 *  memory segment from which the @a physobj item is allocated.
 *
 *  @param physobj
 *    The physmem item to encode. Its contents will contain the encoding
 *    on function return.
 *
 *  @param segment
 *    The segment identifier to allocate from.
 *
 */
INLINE void okl4_physmem_item_setsegment(okl4_physmem_item_t * physobj,
        okl4_word_t segment);

/**
 *  The okl4_physmem_item_getphys() function is used to obtain the base
 *  address of the physmem item specified by the  @a physobj argument.  The physical offset, 
 *  if specified, should be added to this base
 *  address to obtain the real base address of @a physobj.
 *
 */
INLINE okl4_word_t okl4_physmem_item_getphys(okl4_physmem_item_t * physobj);

/**
 *  The okl4_physmem_item_getoffset() function is used to obtain the physical
 *  offset from the physmem item specified by the @a physobj argument.
 *
 */
INLINE okl4_word_t okl4_physmem_item_getoffset(okl4_physmem_item_t * physobj);

/**
 *  The okl4_physmem_item_getsize() function is used to obtain the size of the
 *  physmem item specified by the @a physobj argument.
 *
 */
INLINE okl4_word_t okl4_physmem_item_getsize(okl4_physmem_item_t * physobj);

/**
 *  The struct okl4_physmem_pool_attr is used to represent a physmem
 *  allocator attribute object.  This structure is used for initializing
 *  the physmem allocators, namely the @a physseg @a allocator  
 *  and the @a physpool @a allocator.
 *
 *  Physmem allocators are initialized by deriving a portion of physical
 *  memory from a parent physmem allocator.  The physmem allocator
 *  attribute encodes the parent allocator as well as the range of physical
 *  memory that the child allocator will receive out of the parent.  
 *  Physical memory ranges are encoded into the physmem allocator attribute
 *  using accessor functions.
 *
 */
struct okl4_physmem_pool_attr {
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    okl4_physmem_item_t phys;
    okl4_word_t pagesize;
    OKL4_MICRO(okl4_physmem_segpool_t * parent;)
};

/**
 *  The okl4_physmem_pool_attr_setrange() function is used to encode a
 *  physmem allocator attribute, @a attr, to represent a region of physical
 *  memory with a @a offset address and @a size.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be encoded.
 *
 *  @param offset
 *    The offset address of the physical memory to encode into the
 *    attribute.
 *
 *  @param size
 *    The size of the physical memory to encode into the attribute.
 *
 */
INLINE void okl4_physmem_pool_attr_setrange(okl4_physmem_pool_attr_t * attr,
        okl4_word_t offset, okl4_word_t size);

/**
 *  The okl4_physmem_pool_attr_setsize() function is used to encode a
 *  physmem allocator attribute, @a attr, to represent a region of physical
 *  memory of @a size.  The base address of the region is unspecified.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be encoded.
 *
 *  @param size
 *    The size of the physical memory to encode into the attribute.
 *
 */
INLINE void okl4_physmem_pool_attr_setsize(okl4_physmem_pool_attr_t * attr,
        okl4_word_t size);

/**
 *  The okl4_physmem_pool_attr_setpagesize() function is used to
 *  encode the attribute @a attr to initialize an allocator with a page size
 *  specified by the @a pagesize argument.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be encoded.
 *
 *  @param pagesize
 *    The page size of the physical memory to encode into the attribute. 
 */
INLINE void okl4_physmem_pool_attr_setpagesize(okl4_physmem_pool_attr_t * attr,
        okl4_word_t pagesize);

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_physmem_pool_attr_setparent() function is used to encode the
 *  attribute @a attr to initialize an allocator with the @a parent
 *  allocator.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be encoded.
 *
 *  @param parent
 *    The parent allocator.
 *
 */
INLINE void okl4_physmem_pool_attr_setparent(okl4_physmem_pool_attr_t * attr,
        okl4_physmem_segpool_t * parent);
#endif /* OKL4_KERNEL_MICRO */

/**
 *  The okl4_physpool_attr_init() function is used to initialize a physical
 *  memory allocator attribute specified by the @a attr argument.  
 *  The attribute is initialized to have an
 *  empty interval, and the root physical memory pool as the parent.
 *
 *  The root physical memory pool is constructed by Elfweaver and obtained
 *  via the OKL4 object environment.  It is looked up using the key 
 *  DEFAULT_PHYSSEG_POOL.  Calling this function is equivalent to
 *  calling okl4_physmem_pool_attr_setparent () with the root physmem pool.
 *
 */
void okl4_physmem_pool_attr_init(okl4_physmem_pool_attr_t * attr);

/*
 *  Inlined methods
 */

INLINE void
okl4_physmem_item_setrange(okl4_physmem_item_t * physobj,
        okl4_word_t offset, okl4_word_t size)
{
    assert(physobj != NULL);
    okl4_range_item_setrange(&physobj->range, offset, size);
}

INLINE void
okl4_physmem_item_setsize(okl4_physmem_item_t * physobj,
        okl4_word_t size)
{
    assert(physobj != NULL);
    okl4_range_item_setsize(&physobj->range, size);
}

INLINE void
okl4_physmem_item_setsegment(okl4_physmem_item_t *physobj,
        okl4_word_t segment)
{
    assert(physobj != NULL);
    physobj->segment_id = segment;
}

INLINE okl4_word_t
okl4_physmem_item_getphys(okl4_physmem_item_t * physobj)
{
    assert(physobj != NULL);
    return physobj->paddr;
}

INLINE okl4_word_t
okl4_physmem_item_getoffset(okl4_physmem_item_t * physobj)
{
    assert(physobj != NULL);
    return okl4_range_item_getbase(&physobj->range);
}

INLINE okl4_word_t
okl4_physmem_item_getsize(okl4_physmem_item_t * physobj)
{
    assert(physobj != NULL);
    return okl4_range_item_getsize(&physobj->range);
}

INLINE okl4_word_t
okl4_physmem_item_getsegment(okl4_physmem_item_t * physobj)
{
    assert(physobj != NULL);
    return physobj->segment_id;
}

INLINE void
okl4_physmem_pool_attr_setrange(okl4_physmem_pool_attr_t * attr,
        okl4_word_t offset, okl4_word_t size)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PHYSMEM_POOL_ATTR);

    okl4_physmem_item_setrange(&attr->phys, offset, size);
}

INLINE void
okl4_physmem_pool_attr_setsize(okl4_physmem_pool_attr_t * attr,
        okl4_word_t size)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PHYSMEM_POOL_ATTR);

    okl4_physmem_item_setsize(&attr->phys, size);
}

INLINE void
okl4_physmem_pool_attr_setpagesize(okl4_physmem_pool_attr_t * attr,
        okl4_word_t pagesize)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PHYSMEM_POOL_ATTR);

    attr->pagesize = pagesize;
}

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_physmem_pool_attr_setparent(okl4_physmem_pool_attr_t * attr,
        okl4_physmem_segpool_t * parent)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PHYSMEM_POOL_ATTR);

    attr->parent = parent;
}
#endif /* OKL4_KERNEL_MICRO */


#endif /* !__OKL4__PHYSMEM_OBJECT_H__ */
