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

#ifndef __OKL4__RANGE_H__
#define __OKL4__RANGE_H__

#include <stdio.h>
#include <stddef.h>
#include <okl4/types.h>
#include <okl4/errno.h>
#include <okl4/allocator_attr.h>

/**
 *  @file
 *
 *  The range.h Header.
 *
 *  The range.h header provides the functionality required for making use
 *  of the range allocator strategy.  The range allocator is utilized by
 *  initializing a @a range allocator object, and calling allocation and
 *  deallocation operations on the allocator. Allocation and deallocation
 *  parameters are encoded in @a range item objects.
 *
 *  The following resource allocators depend on the range allocator
 *  strategy:
 *
 *  @li Physical memory segment allocator
 *
 *  @li Virtual memory allocator
 *
 *  @li User TCB allocator 
 *
 */

/**
 *  The okl4_range_item structure is used to encode allocation and
 *  deallocation parameters, as well as allocation results, from the range
 *  allocator.
 *
 */
struct okl4_range_item {
    okl4_word_t base;
    okl4_word_t size;
    okl4_word_t total_size;
    okl4_range_item_t * next;
};

/* Accessor methods */

/*
 *  The methods are divided into two categories, pre-initialisation and
 *  post-initialisation methods.
 *
 *  The pre-initialisation methods only return a defined result if the range
 *  item is newly initialised, or it has been passed into
 *  okl4_range_allocator_destroy().
 *
 *  The post-initialisation methods only return a defined result if the range
 *  item contains an allocation, that is, it has been passed into
 *  okl4_range_allocator_alloc().
 *
 *  Furthermore, if the range item is already initialised, calling any the
 *  pre-initialisation methods on it will destroy its contents!  From then
 *  on, the state of the allocator will be undefined and all further operations
 *  will return undefined results.  Be warned!
 */

/* Pre-initialisation accessor methods */

/**
 *  The okl4_range_item_setrange() function is used to encode a range item
 *  to represent an interval of values with a @a base value and @a size.
 *
 *  This function requires the following arguments:
 *
 *  @param range
 *    The range item to encode. Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param base
 *    The base value of the interval.
 *
 *  @param size
 *    The size of the interval.
 *
 */
INLINE void okl4_range_item_setrange(okl4_range_item_t * range,
        okl4_word_t base, okl4_word_t size);

/**
 *  The okl4_range_item_setsize() function is used to encode a range item
 *  to represent an interval of values of @a size. The base value of the
 *  interval is unspecified.
 *
 *  This function requires the following arguments:
 *
 *  @param range
 *    The range item to encode. Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param size
 *    The size of the interval.
 *
 */
INLINE void okl4_range_item_setsize(okl4_range_item_t * range,
        okl4_word_t size);

/* Post-initialisation accessor methods */

/**
 *  The okl4_range_item_getbase() function is used to decode the base value
 *  out of a range item.
 *
 *  This function requires the following arguments:
 *
 *  @param range
 *    The range item to decode.
 *
 *  This function returns the base value of the range item.
 *
 */
INLINE okl4_word_t okl4_range_item_getbase(okl4_range_item_t * range);

/**
 *  The okl4_range_item_getsize() function is used to decode the size out
 *  of a range item.
 *
 *  This function requires the following arguments:
 *
 *  @param range
 *    The range item to decode.
 *
 *  This function returns the size of the range item.
 *
 */
INLINE okl4_word_t okl4_range_item_getsize(okl4_range_item_t * range);

/**
 *  The okl4_range_allocator structure is used to represent a range
 *  allocator object.  This object is used to keep track of available
 *  values in the allocator, as well as a reference point for making
 *  allocations.
 *
 *  The range allocator object has the following categories of operations:
 *
 *  @li Static memory allocation (Sections okl4_range_decrange() to
 *  okl4_range_decsize())
 *
 *  @li Dynamic memory allocation (okl4_range_size())
 *
 *  @li Initialization and Destruction (Sections okl4_range_init() to
 *  okl4_range_destroy())
 *
 *  @li Allocation and Free (Sections okl4_range_alloc() to
 *  okl4_range_free())
 *
 */
struct okl4_range_allocator {
    okl4_range_item_t head;
};

/* Static Initialisers */

/**
 *  The macro OKL4_RANGE_ALLOCATOR_SIZE() is used to determine the amount
 *  of memory to be dynamically allocated to a range allocator, if the
 *  allocator is initialised with the @a attr attribute.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be used in initialisation.
 *
 *  This function returns the amount of memory to allocate, in bytes. The
 *  result of this function is typically passed into a memory allocation
 *  function such as malloc().
 *
 */
#define OKL4_RANGE_ALLOCATOR_SIZE  \
    (sizeof(struct okl4_range_alocator))

/**
 *  The OKL4_RANGE_ALLOCATOR_DECLARE_OFRANGE() macro is used to declare and
 *  statically allocate memory for a range allocator that can allocate @a
 *  size values, starting from from @a base.  As a result, an @a
 *  uninitialized range allocator with an appripriate amount of memory is
 *  declared as @a name.
 *
 *  The function requires the following arguments:
 *
 *  @param name
 *    Variable name of the allocator.
 *
 *  @param base
 *    The first value available in the allocator.
 *
 *  @param size
 *    The number of values in the allocator.
 *
 *  This macro only handles memory allocation. The initialisation function
 *  must be called separately.
 *
 */
/*
 * The implementation does not currently use @a base or @a size, but is
 * included for future expansion.
 */
#define OKL4_RANGE_ALLOCATOR_DECLARE_OFRANGE(name, base, size)           \
    okl4_range_allocator_t __##name;                     \
    static okl4_range_allocator_t *(name) = &__##name

/**
 *  The macro OKL4_RANGE_ALLOCATOR_DECLARE_OFSIZE() is used to declare and
 *  statically allocate memory for a range allocator that can allocate @a
 *  size number of values.  As a result, an @a uninitialized range
 *  allocator with an appropriate amount of memory is declared as @a name.
 *
 *  @param name
 *    Variable name of the allocator.
 *
 *  @param length
 *    The number of values in the allocator.
 *
 *  This macro only handles memory allocation. The initialisation function
 *  must be called separately.
 *
 */
/*
 * The implementation does not currently use @a size, but is included for
 * future expansion.
 */
#define OKL4_RANGE_ALLOCATOR_DECLARE_OFSIZE(name, size)                 \
    OKL4_RANGE_ALLOCATOR_DECLARE_OFRANGE(name, 0, size)

/* Dynamic Initialisers */

/**
 *  Determine the amount of memory to be dynamically allocated to
 *  a range allocator, if the allocator is initialised with the
 *  @a attr attribute.
 *
 *  @param attr The attribute to be used in initialisation.
 *  @return The amount of memory to allocate, in bytes.
 *
 *  The result of this function is typically passed into a memory
 *  allocation function such as malloc().
 */
#define OKL4_RANGE_ALLOCATOR_SIZE_ATTR(attr) (sizeof(struct okl4_range_allocator))

/**
 *  The okl4_range_allocator_init() function is used to initialise a range
 *  @a allocator with the @a attr attribute.
 *
 *  This function requires the following arguments:
 *
 *  @param allocator
 *    The allocator to initialise.
 *
 *  @param attr
 *    The attribute to initialise with.
 *
 *  An allocator must be initialised before it can be used, unless it is
 *  pre-initialised by Elfweaver.
 *
 */
void okl4_range_allocator_init(okl4_range_allocator_t * allocator,
        okl4_allocator_attr_t * attr);

/**
 *  The okl4_range_allocator_destroy() function is used to destroy a range
 *  @a allocator.
 *
 *  This function requires the following arguments:
 *
 *  @param allocator
 *    The allocator to destroy.
 *
 *  A destroyed allocator may no longer be used. The memory it used to
 *  occupy may be freed or used for other purposes.
 *
 */
void okl4_range_allocator_destroy(okl4_range_allocator_t * allocator);

/* Allocators */

/**
 *  The okl4_range_allocator_alloc() function is used to request an
 *  allocation from @a allocator.  The request is encoded in @a range.
 *
 *  This function requires the following arguments:
 *
 *  @param allocator
 *    The allocator to allocate from.
 *
 *  @param range
 *    The range item that encodes the allocation request. Its contents will
 *    encode the resultant allocation after the function returns.
 *
 *  The @a range item becomes part of the allocator state after this
 *  function returns. It should not be reused or written to until
 *  deallocation is invoked on the item.  This function returns the
 *  following error codes:
 *
 *  @param OKL4_OK
 *    The allocation is successful.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *    The start of requested allocation does not exist in any free region
 *    within the allocator.
 *
 *  @param OKL4_IN_USE
 *    No free region in the allocator is large enough to fulfill the
 *    request.
 *
 */
int okl4_range_allocator_alloc(okl4_range_allocator_t * allocator,
        okl4_range_item_t * range);

/**
 *  The okl4_range_allocator_free() function is used to request a
 *  deallocation from @a allocator.  The request is encoded in @a range.
 *  @a range must be a range item previously allocated from @a allocator.
 *
 *  This function requires the following arguments:
 *
 *  @param allocator
 *    The allocator to allocate from.
 *
 *  @param range
 *    The range item that encodes the deallocation request.
 *
 */
void okl4_range_allocator_free(okl4_range_allocator_t * allocator,
        okl4_range_item_t * range);

/*
 *  Inlined methods
 */

INLINE void
okl4_range_item_setrange(okl4_range_item_t * range, okl4_word_t base,
        okl4_word_t size)
{
    range->base = base;
    range->size = size;
    range->total_size = 0;
    range->next = NULL;
}

INLINE void
okl4_range_item_setsize(okl4_range_item_t * range, okl4_word_t size)
{
    range->base = OKL4_ALLOC_ANONYMOUS;
    range->size = size;
}

INLINE okl4_word_t
okl4_range_item_getbase(okl4_range_item_t * range)
{
    return range->base;
}

INLINE okl4_word_t
okl4_range_item_getsize(okl4_range_item_t * range)
{
    return range->size;
}


#endif /* !__OKL4__RANGE_H__ */
