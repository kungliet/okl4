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

#ifndef __OKL4__VIRTMEM_POOL_H__
#define __OKL4__VIRTMEM_POOL_H__

#include <assert.h>
#include <stddef.h>
#include <okl4/range.h>
#include <okl4/types.h>
#include <okl4/errno.h>
#include <okl4/env.h>

/**
 *  @file
 *
 *  The virtmem_pool.h Header.
 *
 *  The virtmem_pool.h header provides the functionality required for
 *  making user of the virtual memory segments allocator (abbreviated as @a
 *  ``virtmem allocator'' from here onwards). The virtmem allocator uses
 *  the range allocator (range.h) as its underlying allocation strategy.
 *
 *  The virtmem allocator is utilized by initializing a \emph{virtmem
 *  allocator} object (okl4_vmempool_pool()), and calling allocation and
 *  deallocation operations on the allocator.  Allocation and deallocation
 *  parameters are encoded in the @a virtmem item object
 *  (okl4_vmempool_item()).
 *
 */

/**
 *  The okl4_virtmem_pool_attr_t type is used to represent an virtmem
 *  allocator attribute.  This structure is used for initializing virtmem
 *  allocators.
 *
 *  Virtmem allocators are initialized by deriving a portion of virtual
 *  memory out of a parent virtmem allocator.  The virtmem allocator
 *  attribute encodes the parent allocator as well as the range of virtual
 *  memory that the child allocator will receive out of the parent.  By
 *  calling the optional initialization function
 *  (okl4_virtmem_pool_attr_init(), okl4_vmempool_attrinit()), the root
 *  virtmem pool will be used as the parent allocator.
 *
 *  Virtual memory ranges are encoded into the virtmem allocator attribute
 *  using accessor functions described in Sections okl4_vmempool_setrange()
 *  to okl4_vmempool_setsize().
 *
 */
struct okl4_virtmem_pool_attr {
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    okl4_allocator_attr_t range;
    okl4_virtmem_pool_t *parent;
};

/**
 *  The okl4_virtmem_pool_attr_setparent() function is used to encode the
 *  attribute @a attr to initialize an allocator with the @a parent
 *  allocator.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to encode.
 *
 *  @param parent
 *    The parent allocator.
 *
 */
INLINE void okl4_virtmem_pool_attr_setparent(okl4_virtmem_pool_attr_t * attr,
        okl4_virtmem_pool_t * parent);

/**
 *  The okl4_virtmem_pool_attr_setrange() function is used to encode a
 *  virtmem allocator attribute to represent a region of virtual memory
 *  with a @a base address and @a size.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to encode.
 *
 *  @param base
 *    The base address of the virtual memory to encode into the attribute.
 *
 *  @param size
 *    The size of the virtual memory to encode into the attribute.
 *
 */
INLINE void okl4_virtmem_pool_attr_setrange(okl4_virtmem_pool_attr_t * attr,
        okl4_word_t base, okl4_word_t size);

/**
 *  The okl4_virtmem_pool_attr_setsize() function is used to encode a
 *  virtmem allocator attribute to represent a region of virtual memory of
 *  @a size.  The base address of the region is unspecified.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to encode.
 *
 *  @param size
 *    The size of the virtual memory to encode into the attribute.
 *
 */
INLINE void okl4_virtmem_pool_attr_setsize(okl4_virtmem_pool_attr_t * attr,
        okl4_word_t size);

/**
 *  The okl4_virtmem_pool_attr_init() function is used to initialize a
 *  virtual memory allocator attribute.  The attribute is initialized to
 *  have an empty interval, and the root virtual memory pool as the parent.
 *
 *  The root virtual memory pool is constructed by Elfweaver and obtained
 *  via the OKL4 object environment.  It is looked up using the key ``{\tt
 *  ROOT_VIRTMEM_POOL}''.  Calling this function is equivalent to calling
 *  okl4_virtmem_pool_attr_setparent() with the root virtmem pool.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to initialise.
 *
 */
void okl4_virtmem_pool_attr_init(okl4_virtmem_pool_attr_t * attr);

/**
 *  @struct okl4_virtmem_item
 *
 *  The okl4_virtmem_item_t type is used to represent the @a virtmem item,
 *  a region of virtual memory. This represents OKL4's view of virtual
 *  memory at the user level.
 *
 *  The virtmem item is used to encode allocation and deallocation
 *  parameters, as well as allocation results, from the virtmem allocator.
 *  The virtmem item uses the @a range item object (okl4_range_item()) as
 *  the underlying implementation.
 *
 *  The virtmem item is encoded and decoded using the same functions as the
 *  range item object.  The functions for manipulating range items are
 *  described in Sections okl4_range_setrange() to okl4_range_getsize().
 *
 */

/**
 *  The okl4_virtmem_pool_t type is used to represent a virtmem allocator
 *  object.  This object is used to keep track of available virtual memory
 *  in a given cell, as well as a reference point for making virtual memory
 *  allocations.  The allocations may be variable-sized.
 *
 *  It should be noted that although the library user is free to create
 *  virtmem allocators with arbitary virtual memory, such allocators are
 *  effectively useless if the calling protection domain does not actually
 *  have access to the corresponding virtual memory.
 *
 *  The virtmem allocator object has the following categories of
 *  operations:
 *
 *  @li Dynamic memory allocation (okl4_vmempool_size());
 *
 *  @li Allocator derivation (okl4_vmempool_derive());
 *
 *  @li Initialization and Destruction (Sections okl4_vmempool_init() and
 *  okl4_vmempool_destroy()); and
 *
 *  @li Allocation and Free (Sections okl4_vmempool_alloc() and
 *  okl4_vmempool_free()).
 *
 */
struct okl4_virtmem_pool {
    okl4_virtmem_item_t virt;
    okl4_range_allocator_t allocator;
    okl4_range_allocator_t *parent;
};

/*
 * Static Declaration Macros
 */

/**
 *  The macro OKL4_VIRTMEM_POOL_SIZE() is used to determine the amount of
 *  memory to be dynamically allocated to a virtual memory allocator, if
 *  the allocator is initialised with the @a attr attribute.
 *
 *  @param attr
 *    The attribute to be used in initialisation.
 *
 *  This function returns the amount of memory to allocate, in bytes. The
 *  result of this function is typically passed into a memory allocation
 *  function such as malloc().
 *
 */
#define OKL4_VIRTMEM_POOl_SIZE(name)             \
    sizeof(struct okl4_virtmem_pool)

/**
 *  No comment.
 */
#define OKL4_VIRTMEM_POOL_DECLARE_OFRANGE(name, base, size)     \
    okl4_virtmem_pool_t __##name;                               \
    static okl4_virtmem_pool_t *(name) = &__##name

/**
 *  Determine the amount of memory to be dynamically allocated to
 *  a virtual memory allocator, if the allocator is initialised with the
 *  @a attr attribute.
 *
 *  @param attr The attribute to be used in initialisation.
 *  @return The amount of memory to allocate, in bytes.
 *
 *  The result of this function is typically passed into a memory
 *  allocation function such as malloc().
 */
#define OKL4_VIRTMEM_POOL_SIZE_ATTR(attr)        sizeof(struct okl4_virtmem_pool)

/*
 * Accessor methods
 */

/**
 *  The okl4_virtmem_pool_alloc_pool() function is used to initialise a
 *  virtual memory allocator that derives its allocations from a parent
 *  allocator.
 *
 *  The virtmem pool attribute @a attr encodes the parent allocator to
 *  derive the child allocator, @a pool, from.  @a attr also encodes a
 *  range of virtual addresses that @a pool will receive from the parent
 *  allocator.  The range encoded in @a attr may be named or anonymous.
 *  The function makes an allocation request on the parent allocator for
 *  the encoded range, and initializes @a pool with this range as the
 *  initial set of resources. okl4_virtmem_pool_init() does not need to be
 *  further invoked on @a pool since this is done implicitly in this
 *  function.
 *
 *  The intended method for creating new virtmem allocators is to always
 *  derive from existing virtmem allocators.  The initial root virtmem
 *  allocator for any given protection domain contains all the virtual
 *  memory available to that cell, and this is the starting point for
 *  deriving virtmem allocators.  The initial root virtmem allocator is
 *  constructed by Elfweaver and obtained via the OKL4 object environment.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    The allocator to initialise.
 *
 *  @param attr
 *    The attribute to initialise with.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    The initialisation is successful.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *    Same meaning as okl4_range_allocator_alloc().
 *
 *  @param OKL4_IN_USE
 *    Same meaning as okl4_range_allocator_alloc().
 *
 */
int okl4_virtmem_pool_alloc_pool(okl4_virtmem_pool_t * pool,
        okl4_virtmem_pool_attr_t * attr);

/**
 *  The function okl4_virtmem_pool_init() is used to initialise a virtual
 *  memory allocator with the @a attr attribute.
 *
 *  This function expects the @a attr to encode a range of virtual
 *  addresses that @a pool will have available in its allocation pool. The
 *  range of virtual addresses must be named.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    The allocator to initialise.
 *
 *  @param attr
 *    The attribute to initialise with.
 *
 */
void okl4_virtmem_pool_init(okl4_virtmem_pool_t * pool,
        okl4_virtmem_pool_attr_t * attr);

/**
 *  The okl4_virtmem_pool_destroy() function is used to destroy a virtual
 *  memory allocator.  A destroyed allocator may no longer be used. The
 *  memory it used to occupy may be freed or used for other purposes.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    The allocator to destroy.
 *
 */
void okl4_virtmem_pool_destroy(okl4_virtmem_pool_t * pool);

/**
 *  The okl4_virtmem_pool_alloc() function is used to request an allocation
 *  from @a pool.  The request is encoded in @a range.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    The allocator to allocate from.
 *
 *  @param range
 *    The virtmem item that encodes the allocation request. Its contents
 *    will encode the resultant allocation after the function returns.
 *
 *  This funciton returns the following error conditions:
 *
 *  @param OKL4_OK
 *    The allocation is successful.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *    Same meaning as okl4_range_allocator_alloc().
 *
 *  @param OKL4_IN_USE
 *    Same meaning as okl4_range_allocator_alloc().
 *
 */
int okl4_virtmem_pool_alloc(okl4_virtmem_pool_t * pool,
        okl4_range_item_t * range);

/**
 *  The okl4_virtmem_pool_free() function is used to request a deallocation
 *  from @a pool.  The request is encoded in @a range.  @a range must be a
 *  virtmem item previously allocated from @a pool.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    The allocator to deallocate from.
 *
 *  @param range
 *    The virtmem item that encodes the deallocation request.
 *
 */
void okl4_virtmem_pool_free(okl4_virtmem_pool_t * pool,
        okl4_range_item_t * range);

/**
 *  No comment.
 */
INLINE okl4_word_t okl4_virtmem_pool_getbase(okl4_virtmem_pool_t * pool);

/*
 *  Inlined methods
 */

INLINE void
okl4_virtmem_pool_attr_setrange(okl4_virtmem_pool_attr_t *attr,
        okl4_word_t base, okl4_word_t size)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_VIRTMEM_POOL_ATTR);

    okl4_allocator_attr_setrange(&attr->range, base, size);
}

INLINE void
okl4_virtmem_pool_attr_setsize(okl4_virtmem_pool_attr_t *attr,
        okl4_word_t size)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_VIRTMEM_POOL_ATTR);

    okl4_allocator_attr_setsize(&attr->range, size);
}

INLINE void
okl4_virtmem_pool_attr_setparent(okl4_virtmem_pool_attr_t *attr,
        okl4_virtmem_pool_t *parent)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_VIRTMEM_POOL_ATTR);

    attr->parent = parent;
}

INLINE okl4_word_t
okl4_virtmem_pool_getbase(okl4_virtmem_pool_t *pool)
{
    assert(pool != NULL);
    return okl4_range_item_getbase(&pool->virt);
}

INLINE okl4_word_t
okl4_virtmem_pool_getsize(okl4_virtmem_pool_t *pool)
{
    assert(pool != NULL);
    return okl4_range_item_getsize(&pool->virt);
}

#endif /* !__OKL4__VIRTMEM_POOL_H__ */

