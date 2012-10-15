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

#ifndef __OKL4__BITMAP_H__
#define __OKL4__BITMAP_H__

#include <okl4/types.h>
#include <okl4/errno.h>
#include <okl4/allocator_attr.h>
#include <okl4/bitmap.h>

/**
 *  @file
 *
 *  The bitmap.h Header.
 *
 *  The bitmap.h header provides the functionality required for using
 *  the bitmap allocator strategy.  The bitmap allocator is utilized by
 *  initializing a @a bitmap allocator object and calling allocation and
 *  deallocation operations on the allocator.  Allocation and deallocation
 *  parameters are encoded using @a bitmap item objects.
 *
 *  The following resource allocators use the bitmap allocator
 *  strategy:
 *
 *  @li Kernel clist id allocator
 * 
 *  @li Kernel mutex id allocator 
 *
 *  @li Kernel space id allocator 
 *
 *  @li Physical memory page allocator 
 *
 *  @li User TCB allocator 
 *
 */

/**
 *  The okl4_bitmap_item structure is used to encode allocation and
 *  deallocation parameters in addition to storing allocation results 
 *  from the bitmap allocator.
 *   
 */
struct okl4_bitmap_item {
    okl4_word_t unit;
};

/* Pre-initialisation accessor methods */

/**
 *  The okl4_bitmap_item_setany() function is used to set the bitmap item 
 *  specified by the @item argument such that it specifies that the
 *  allocation may come from any location in the allocator.
 *
 */
INLINE void okl4_bitmap_item_setany(okl4_bitmap_item_t * item);

/**
 *  The okl4_bitmap_item_setunit() function is used to encode a bitmap 
 *  item specified by the @a item argument, such that it specifies that 
 *  allocation must come from the location specified by the @a unit 
 *  argument within the allocator.
 *
 *  The function requires the following arguments:
 *
 *  @param item
 *    The bitmap item to encode. Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param unit
 *    The location from which the bitmap allocator should allocate a value.
 *
 */
INLINE void okl4_bitmap_item_setunit(okl4_bitmap_item_t * item,
        okl4_word_t unit);

/* Post-initialisation accessor methods */

/**
 *  The okl4_bitmap_item_getunit() function is used to obtain the encoded
 *  location of the bitmap item specified by the @a item argument.
 *
 */
INLINE okl4_word_t okl4_bitmap_item_getunit(okl4_bitmap_item_t * item);

/**
 *  The okl4_bitmap_allocator structure is used to represent a
 *  bitmap allocator object.  The bitmap allocator object is used to keep track of
 *  available values in the allocator, as well as acting as a reference point for
 *  making allocations.
 *
 *  The following operations may be performed on a bitmap allocator object:
 *
 *  @li Static memory allocation described in Sections
 *  OKL4_BITMAP_ALLOCATOR_DECLARE_OFRANGE() and 
 *  OKL4_BITMAP_ALLOCATOR_DECLARE_OFSIZE().
 *
 *  @li Dynamic memory allocation described in Section OKL4_BITMAP_ALLOCATOR_SIZE().
 *
 *  @li Initialization and Destruction described in Sections okl4_bitmap_allocator_init() and
 *  okl4_bitmap_allocator_destroy().
 *
 *  @li Query described in Section okl4_bitmap_allocator_isallocated().
 *
 *  @li Allocation and Free described in Sections okl4_bitmap_allocator_alloc() and
 *  okl4_bitmap_allocator_free().
 *
 */
struct okl4_bitmap_allocator {
    okl4_word_t base;
    okl4_word_t size;
    okl4_word_t pos_guess;
    okl4_word_t data[1]; /* Variable sized array */
};

/* Static Initialisers */

/**
 *  The OKL4_BITMAP_ALLOCATOR_DECLARE_OFRANGE() macro is used to declare
 *  and statically allocate memory for a bitmap allocator with the name 
 *  specified by the @a name argument, capable of allocation a 
 *  @a size number of values starting from @a base. 
 *
 *  This function requires the following arguments:
 *
 *  @param name
 *    Name of the allocator.
 *
 *  @param base
 *    The first value available in the allocator.
 *
 *  @param size
 *    The number of values in the allocator.
 *
 *  Note that the resulting bitmap allocator is uninitialized. The initialization function
 *  must be called separately.
 *
 */
#define OKL4_BITMAP_ALLOCATOR_DECLARE_OFRANGE(name, base, size)                \
    okl4_word_t __##name[OKL4_BITMAP_ALLOCATOR_SIZE(size)];                    \
    static okl4_bitmap_allocator_t *(name) =                                   \
            (okl4_bitmap_allocator_t *)(void *) __##name

/**
 *  Declare and statically allocate memory for a bitmap allocator
 *  that can allocate values from @a base to @a base + @a size.
 *
 *  @param name Variable name of the allocator.
 *  @param base The base limit of the allocator.
 *  @param size The size limit of the allocator.
 *
 *  This macro only handles memory allocation.  The initialisation
 *  function must be called separately.
 */

/**
 *  The OKL4_BITMAP_ALLOCATOR_DECLARE_OFSIZE() macro is used to declare and
 *  statically allocate memory for a bitmap allocator with the name specified 
 *  by the @a name argument, capable of allocating a
 *  @a size number of values.   
 *
 *  This function requires the following arguments:
 *
 *  @param name
 *    Name of the allocator.
 *
 *  @param size
 *    The number of values in the allocator.
 *
 *  Note that the resulting bitmap allocator is uninitialized. The initialization function
 *  must be called separately.
 *
 */
#define OKL4_BITMAP_ALLOCATOR_DECLARE_OFSIZE(name, size)                       \
    OKL4_BITMAP_ALLOCATOR_DECLARE_OFRANGE(name, 0, size)

/* Dynamic Initialisers */

/**
 *  The OKL4_BITMAP_ALLOCATOR_SIZE() macro is used to determine the
 *  amount of memory required to dynamically allocate a bitmap allocator
 *  capable of allocating a @a size number of items.
 *
 */
/*lint -emacro(778, OKL4_BITMAP_ALLOCATOR_SIZE) */
/*lint -emacro(941, OKL4_BITMAP_ALLOCATOR_SIZE) */
#define OKL4_BITMAP_ALLOCATOR_SIZE(size)                                       \
    ((((size) + (OKL4_WORD_T_BIT - 1)) / OKL4_WORD_T_BIT - 1)                  \
    * sizeof(okl4_word_t) + sizeof(struct okl4_bitmap_allocator))

/**
 *  The OKL4_BITMAP_ALLOCATOR_SIZE_ATTR() macro is used to
 *  determine the amount of memory required to dynamically allocate
 *  a bitmap allocator, where the allocator is initialized with the
 *  @aattr attribute. This function returns the amount of memory 
 *  required to allocate the bitmap allocator in bytes. 
 *
 */
#define OKL4_BITMAP_ALLOCATOR_SIZE_ATTR(attr)                                  \
    OKL4_BITMAP_ALLOCATOR_SIZE((attr)->size)

/**
 *  The okl4_bitmap_allocator_init() function is used to initialize the 
 *  bitmap allocator specified by the @a allocator argument with the attributes
 *  specified by the @attr argument.
 *
 *  This function requires the following arguments:
 *
 *  @param allocator
 *    The allocator to initialize.
 *
 *  @param attr
 *    The attributes with which to initialize the allocator.
 *
 *  An allocator must be initialized prior to use, unless initialized by Elfweaver.
 *
 */
void okl4_bitmap_allocator_init(okl4_bitmap_allocator_t * allocator,
        okl4_allocator_attr_t * attr);

/**
 *  The okl4_bitmap_allocator_destroy() function is used to destroy an existing  
 *  bitmap allocator specified by the @a allocator argument.
 *
 *  A destroyed allocator may no longer be used. 
 *
 */
void okl4_bitmap_allocator_destroy(okl4_bitmap_allocator_t * allocator);

/**
 *  The okl4_bitmap_allocator_isallocated() function is used to check
 *  whether the value specified by the @a item argument is currently allocated 
 *  by the bitmap allocator specified by the @a allocator argument.  
 *
 *  This function requires the following arguments:
 *
 *  @param allocator
 *    The allocator to query.
 *
 *  @param item
 *    The bitmap item queried.
 *
 *  This function returns 0 if the encoded value is free, and 1 if
 *  it is currently allocated.
 *
 */
int okl4_bitmap_allocator_isallocated(okl4_bitmap_allocator_t * allocator,
        okl4_bitmap_item_t * item);

/* Allocators */

/**
 *  The okl4_bitmap_allocator_alloc() function is used to request an
 *  allocation specified by the @a item argument from the bitmap allocator 
 *  specified by the @a allocator argument.  
 *
 *  This function requires the following arguments:
 *
 *  @param allocator
 *    The allocator from which the allocation is requested.
 *
 *  @param item
 *    The allocation request. This argument will also contain the resultant 
 *    allocation on function return.
 *
 *  The bitmap @a item may be reused on subsequent allocation calls. This
 *  function returns the following error codes:
 *
 *  @retval OKL4_OK
 *    The allocation was successful.
 *
 *  @retval OKL4_ALLOC_EXHAUSTED
 *    There are no more values available for allocation.
 *
 *  @retval OKL4_IN_USE
 *    The requested value is currently allocated.
 *
 *  @retval OKL4_OUT_OF_RANGE
 *    The requested value is not contained in the initial allocatable pool of the
 *    allocator.
 *
 */
int okl4_bitmap_allocator_alloc(okl4_bitmap_allocator_t * allocator,
        okl4_bitmap_item_t * item);

/**
 *  The okl4_bitmap_allocator_free() function is used to request a
 *  deallocation specified by the @a item argument from the bitmap allocator
 *  specified by the @a allocator argument.  
 *
 *  This function requires the following arguments:
 *
 *  @param allocator
 *    The bitmap allocator from which to deallocate.
 *
 *  @param item
 *    The deallocation request.
 *
 */
void okl4_bitmap_allocator_free(okl4_bitmap_allocator_t * allocator,
        okl4_bitmap_item_t * item);

/*
 *  Inline methods
 */

INLINE void
okl4_bitmap_item_setany(okl4_bitmap_item_t * item)
{
    assert (item != NULL);
    item->unit = OKL4_ALLOC_ANONYMOUS;
}

INLINE void
okl4_bitmap_item_setunit(okl4_bitmap_item_t * item, okl4_word_t unit)
{
    assert (item != NULL);
    item->unit = unit;
}

INLINE okl4_word_t
okl4_bitmap_item_getunit(okl4_bitmap_item_t * item)
{
    assert (item != NULL);

    return item->unit;
}

INLINE void
okl4_bitmap_allocator_getattribute(okl4_bitmap_allocator_t *allocator,
        okl4_allocator_attr_t *attr)
{
    assert(allocator != NULL);
    assert(attr != NULL);

    attr->base = allocator->base;
    attr->size = allocator->size;
}

#endif /* !__OKL4__BITMAP_H__ */
