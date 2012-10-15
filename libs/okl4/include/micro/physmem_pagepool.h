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

#ifndef __OKL4__PHYSMEM_PAGEPOOL_H__
#define __OKL4__PHYSMEM_PAGEPOOL_H__

#include <assert.h>
#include <stddef.h>
#include <okl4/bitmap.h>
#include <okl4/types.h>
#include <okl4/errno.h>
#include <okl4/physmem_item.h>

/**
 *  @file
 *
 *  The physmem_pagepool.h Header.
 *
 *  The physmem_pagepool.h header provides the functionality required
 *  for making use of the physical memory pages allocator (abbreviated as
 *  @a ``physpage allocator'' from here onwards).  The physpage allocator
 *  uses the bitmap allocator (bitmap.h) as its underlying allocation
 *  strategy.
 *
 *  The physpage allocator is utilized by initializing a physpage
 *  allocator objec and calling allocation and
 *  deallocation operations on the allocator.  Allocation and deallocation
 *  parameters are encoded using a physmem object.
 *
 *
 */

/**
 *  The okl4_physmem_pagepool type is used to represent a physpage
 *  allocator object.  This object is used to keep track of available
 *  physical memory in the cell, and allocates on a page-by-page basis.
 *
 *  The initial pool of physical memory available to any given cell is
 *  controlled by the Elfweaver system configuration.  Although the library
 *  user is free to create physpage allocators with arbitary physical
 *  memory, they cannot be used if the library user
 *  does not actually own the corresponding physical memory.
 *
 *  The following operations may be performed on a physpage allocator object:
 *
 *  @li Dynamic memory allocation.
 *
 *  @li Allocator derivation.
 *
 *  @li Initialization and Destruction.
 *
 *  @li Query.
 *
 *  @li Allocation and Free.
 *
 */
struct okl4_physmem_pagepool {
    okl4_physmem_item_t phys;
    okl4_word_t pagesize;
    okl4_range_allocator_t * parent;
    okl4_bitmap_allocator_t allocator;
};

/* Static Declaration Macros */

/**
 *  The OKL4_PHYSMEM_PAGEPOOL_SIZE() macro is used to determine the amount
 *  of memory required to dynamically allocate a physpage allocator, if the
 *  allocator is initialized with the @a attr attribute, in bytes.
 *
 */
#define OKL4_PHYSMEM_PAGEPOOL_SIZE(size, pagesize)                          \
    (sizeof(okl4_physmem_pagepool_t) +                                      \
        OKL4_BITMAP_ALLOCATOR_SIZE((size)/(pagesize)))

/**
 *  
 */
#define OKL4_PHYSMEM_PAGEPOOL_DECLARE_OFRANGE(name, base, size, pagesize)   \
    okl4_word_t __##name[OKL4_PHYSMEM_PAGEPOOL_SIZE(size,                   \
            pagesize)];                                                     \
    static okl4_physmem_pagepool_t *(name) =                                \
            (okl4_physmem_pagepool_t *) __##name

/**
 *  The OKL4_PHYSMEM_PAGEPOOL_SIZE_ATTR() macro is used to  
 *  determine the amount of memory required to dynamically allocate
 *  a physical pages allocator, if the allocator is initialized with the
 *  @a attr attribute.
 *
 */
#define OKL4_PHYSMEM_PAGEPOOL_SIZE_ATTR(attr)                               \
    OKL4_PHYSMEM_PAGEPOOL_SIZE(okl4_physmem_item_getsize(&((attr)->phys)),  \
            (attr)->pagesize)

/**
 *  The okl4_physmem_pagepool_alloc_pool() function is used to initializ0e a
 *  physpage allocator that derives its allocations from a parent
 *  allocator.
 *
 *  The physmem pool attribute @a attr encodes the parent allocator 
 *  from which the child allocator, @a pool, is derived.  @a attr also encodes a
 *  range of physical addresses that @a pool will receive from the parent
 *  allocator.  The range encoded in @a attr may be named or anonymous.
 *  The function makes an allocation request on the parent allocator for
 *  the encoded range and initializes @a pool with this range as the
 *  initial available set of resources. 
 *
 *  Note that physpage allocators always derive from physseg
 *  allocators.  
 *
 *  The intended usage for creating new physpage allocators is to always
 *  derive from existing physseg allocators.  The initial root physseg
 *  allocator for any given cell contains all  physical memory available
 *  to that cell. This is the starting point for deriving physpage
 *  allocators.  The initial root physseg allocator is constructed by
 *  Elfweaver and obtained via the OKL4 object environment.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    The allocator to initialize.
 *
 *  @param attr
 *    The attribute to initialize with.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Initialization was successful.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *    There are no more values available for allocation.
 *
 *  @param OKL4_IN_USE
 *    The requested value is currently allocated.
 *
 */
int okl4_physmem_pagepool_alloc_pool(okl4_physmem_pagepool_t * pool,
        okl4_physmem_pool_attr_t * attr);

/**
 *  The okl4_physmem_pagepool_init() function is used to initialize a
 *  physical page allocator with the @a attr attribute. 
 *
 *  This function expects the @a attr to encode a range of physical
 *  addresses and that the @a pool will have  available for allocation. The
 *  range of physical addresses must be named.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    The allocator to initialize.
 *
 *  @param attr
 *    The attribute to initialize with.
 *
 */
void okl4_physmem_pagepool_init(okl4_physmem_pagepool_t * pool,
        okl4_physmem_pool_attr_t * attr);

/**
 *  The function okl4_physmem_pagepool_destroy() is used to destroy a
 *  physical pages allocator specified by the @a pool argument.  
 *  This allocator may no longer be used and the memory it used to 
 *  occupy may be freed or used for other purposes.
 *
 */
void okl4_physmem_pagepool_destroy(okl4_physmem_pagepool_t * pool);

/**
 *  The okl4_physmem_pagepool_alloc() function is used to request an
 *  allocation from @a pool.  The request is encoded in @a phys.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    The allocator to allocate from.
 *
 *  @param phys
 *    The physmem item that encodes the allocation request. Its contents
 *    will encode the resultant allocation on function return.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Allocation was successful.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *    There are no more values available for allocation.
 *
 *  @param OKL4_IN_USE
 *   The requested value is currently allocated.
 *
 */
int okl4_physmem_pagepool_alloc(okl4_physmem_pagepool_t * pool,
        okl4_physmem_item_t * phys);

/**
 *  The okl4_physmem_pagepool_free() function is used to request a
 *  deallocation from @a pool.  The request is encoded in @a phys.  @a phys
 *  must be a physmem item previously allocated from @a pool.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    The allocator to deallocate from.
 *
 *  @param phys
 *    The physmem item that encodes the deallocation request.
 *
 */
void okl4_physmem_pagepool_free(okl4_physmem_pagepool_t * pool,
        okl4_physmem_item_t * phys);

/**
 *  The okl4_physmem_pagepool_getphys() function is used to retrieve the
 *  range of physical addresses that are available in the allocation pool
 *  specified by the @a pool argument at the time of initialization.
 *  This function returns the physical memory item that encodes the range
 *  of available physical addresses.
 *
 */
INLINE okl4_physmem_item_t * okl4_physmem_pagepool_getphys(
        okl4_physmem_pagepool_t * pool);

/**
 *  The okl4_physmem_pagepool_getpagesize() function is used to determine
 *  the page size of the physpage allocator specified by the @a pool argument. 
 *  An allocation from @a pool is always a physical memory page of the
 *  returned size.
 *
 */
INLINE okl4_word_t okl4_physmem_pagepool_getpagesize(okl4_physmem_pagepool_t * pool);

/*
 * Inlined methods
 */

INLINE okl4_physmem_item_t *
okl4_physmem_pagepool_getphys(okl4_physmem_pagepool_t * pool)
{
    assert(pool != NULL);
    return &pool->phys;
}

INLINE okl4_word_t
okl4_physmem_pagepool_getpagesize(okl4_physmem_pagepool_t * pool)
{
    assert(pool != NULL);
    return pool->pagesize;
}


#endif /* !__OKL4__PHYSMEM_PAGEPOOL_H__ */
