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

#ifndef __OKL4__KCLISTID_POOL_H__
#define __OKL4__KCLISTID_POOL_H__

#include <okl4/bitmap.h>
#include <okl4/types.h>

/**
 *  @file
 *
 *  The kclistid_pool.h Header.
 *
 *  The kclistid_pool.h header provides the functionality required for
 *  using the kclist id allocator.  The kclist id allocator uses
 *  the bitmap allocator as its underlying allocation strategy.
 *
 *  The kclist id allocator is utilized by initializing a kclist id
 *  allocator object and calling allocation and deallocation operations
 *  on the allocator.  Allocation and deallocation parameters are encoded
 *  in kclist identifier object.
 *
 *  It should be noted that there are currently no accessor functions
 *  defined for the kclist id object.  The kclist identifier object is defined in
 *  the okl4/types.h header described in Chapter types.h. 
 *
 */

/**
 *  @struct okl4_kclistid_pool
 *
 *  The okl4_kclistid_pool type is used to represent a kernel clist id
 *  allocator object.  This object is used by cell managers to keep track
 *  of available kernel clist ids.
 *  The initial pool of kclist identifiers available to any given cell is
 *  controlled by the Elfweaver system configuration.
 *
 */

/* Static Initialisers */

/**
 *  The OKL4_KCLISTID_POOL_SIZE() macro is used to determine the 
 *  amount of memory required to allocate a kclist identifier pool 
 *  with a @a size number of items in bytes. 
 *
 */
#define OKL4_KCLISTID_POOL_SIZE(size)                               \
        OKL4_BITMAP_ALLOCATOR_SIZE(size)

/**
 * The OKL4_KCLISTID_POOL_DECLARE_OFRANGE() macro is used to declare 
 * and statically allocate memory for a kclist identifier pool capable 
 * of allocating identifiers from @a base to @a base + @a size.
 *
 * This function requires the following arguments:
 *
 * @param name Variable name for the pool.
 * @param base The base limit of the pool.
 * @param size The size limit of the pool.
 *
 * This macro only handles memory allocation. The initialization
 * function must be called separately.
 */
#define OKL4_KCLISTID_POOL_DECLARE_OFRANGE(name, base, size)        \
        OKL4_BITMAP_ALLOCATOR_DECLARE_OFRANGE(name, base, size)

/**
 *  The OKL4_KCLIST_ID_POOL_DECLARE_OFSIZE() is used to declare and 
 *  statically allocate memory for a kclist identifier pool
 *  capable of allocating a @a size number of values.
 *
 *  This function requires the following arguments:
 *
 *  @param name Variable name of the pool.
 *  @param size Size of the pool.
 *
 *  This macro only handles memory allocation.  The initialization
 *  function must be called separately.
 */
#define OKL4_KCLIST_ID_POOL_DECLARE_OFSIZE(name, size)              \
        OKL4_BITMAP_ALLOCATOR_DECLARE_OFSIZE(name, size)

/* Dynamic initialisers. */

/**
 *  The OKL4_KCLISTID_POOL_SIZE_ATTR() macro is used to determine the 
 *  amount of memory required to dynamically allocate a 
 *  kclist identifier pool, if the pool is initialized with the
 *  attribute specifid by the @a attr argument, in bytes.
 *
 */
#define OKL4_KCLISTID_POOL_SIZE_ATTR(attr)                          \
        OKL4_BITMAP_ALLOCATOR_SIZE(attr)

/* The Kernel Cabability-list Identifier allocator methods */

/**
 *  The okl4_kclistid_pool_init() function is used to
 *  initisalise a kclist allocator with the @a attr attribute.
 *  This function provides a simple wrapper around the 
 *  okl4_bitmap_allocator_init () function.
 *
 *  This function requires the following arguments:
 *
 *  @param pool Pool to be initialized.
 *  @param attr Attributes with which to initialize.
 *
 *  A pool must be initialized prior to use, unless it ia
 *  pre-initialized by Elfweaver.
 * 
 */

INLINE void okl4_kclistid_pool_init(okl4_kclistid_pool_t *pool,
        okl4_allocator_attr_t *attr);

/**
 *  The okl4_kclistid_allocany() function is used to allocate any kclist identifier
 *  from the allocator specified by the @a pool argument.  This function returns 
 *  the allocated identifier using the @a id argument.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    Allocator from which the allocation is requested.
 *
 *  @param id
 *    kclist identifier containing the resultant allocation on
 *    function return.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Allocation was successful.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *   There are no more values available for allocation.
 *
 */
int okl4_kclistid_allocany(okl4_kclistid_pool_t * pool, okl4_kclistid_t * id);

/**
 *  The okl4_kclistid_allocunit() function is used to allocate a specific
 *  kclist identifier specified by the @a id argument from the kclist identifier 
 *  pool specified by the @a pool argument. The requested value is encoded
 *  in the @a id argument.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    Allocator from which the allocation is requested.
 *
 *  @param id
 *    kclist identifier that encodes the allocation request. Its contents will
 *    encode the resultant allocation on function return.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Allocation was successful.
 *
 *  @param OKL4_IN_USE
 *    The requested value is currently allocated.
 *
 *  @param OKL4_OUT_OF_RANGE
 *    The requested value is not contained in the initial allocatable pool of the
 *    allocator.
 *
 */
int okl4_kclistid_allocunit(okl4_kclistid_pool_t * pool, okl4_kclistid_t * id);

/**
 *  The okl4_kclistid_free() function is used to deallocate a kclist specified 
 *  by the @a id argument from the allocator specified by the @a pool argument.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    Allocator from which the deallocate is to be made.
 *
 *  @param id
 *    kclist identifier to be deallocated.
 *
 */
void okl4_kclistid_free(okl4_kclistid_pool_t * pool, okl4_kclistid_t id);

/**
 *  The okl4_kclistid_getattribute() function is used to obtain the
 *  attributes of the kclist identifier pool specified by the @a pool argument.
 *  These attributes are encoded in the attribute structure specified by the 
 *  @a attr argument. 
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    kclist identifier allocator pool.
 *
 *  @param attr
 *    Attribute structure to be encoded with the allocator attributes.
 *
 */
void okl4_kclistid_getattribute(okl4_kclistid_pool_t * pool,
        okl4_allocator_attr_t * attr);

/**
 *  The okl4_kclistid_isallocated() function is used to determine whether a
 *  kclist identifier is currently allocated. This function returns 1 if the 
 *  specified kclist identifier is currently allocated and 0 if it is not.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    kclist identifier allocator pool.
 *
 *  @param id
 *    The kclist identifier. 
 * 
 *  This function returns the following error codes:
 *
 *  @retval OKL4_OUT_OF_RANGE
 *    The requested value is not contained in the initial allocatable pool of the
 *    allocator.
 *
 */
int okl4_kclistid_isallocated(okl4_kclistid_pool_t * pool, okl4_kclistid_t id);


INLINE void
okl4_kclistid_pool_init(okl4_kclistid_pool_t *pool,
        okl4_allocator_attr_t *attr)
{
    assert(pool != NULL);
    assert(attr != NULL);

    okl4_bitmap_allocator_init(pool, attr);
}

#endif /* !__OKL4__KCLISTID_POOL_H__ */
