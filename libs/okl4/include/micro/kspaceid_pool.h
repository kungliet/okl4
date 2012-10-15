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

#ifndef __OKL4__KSPACEID_POOL_H__
#define __OKL4__KSPACEID_POOL_H__

#include <okl4/bitmap.h>
#include <okl4/types.h>

/**
 *  @file
 *
 *  The kspaceid_pool.h.
 *
 *  The kspaceid_pool.h header provides the functionality required for
 *  using the kspace identifier allocator, which uses
 *  a bitmap allocator as its underlying allocation strategy.
 *
 *  The kspace identifier allocator is utilized by initializing a kspace identifier
 *  allocator object and calling allocation and deallocation operations
 *  on the allocator.  Allocation and deallocation parameters are encoded
 *  using a kspace identifier object.
 *
 *  It should be noted that there are currently no accessor functions
 *  defined for the kspace identifier object.  The kspace identifier object is defined in
 *  the okl4/types.h header. 
 *
 */

/* Static Initialisers */

/**
 * The OKL4_KSPACEID_POOL_SIZE() macro is used to determine the amount of memory 
 * required to allocate a kspace identifier pool containing size items, in bytes.
 *
 */

#define OKL4_KSPACEID_POOL_SIZE(size)                               \
        OKL4_BITMAP_ALLOCATOR_SIZE(size)

/**
 * The OKL4_KSPACEID_POOL_DECLARE_OFRANGE() macro is used to declare and 
 * statically allocate memory for a kspace identifier pool capable of 
 * allocating identifiers from @a base to a base + @a size.
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
#define OKL4_KSPACEID_POOL_DECLARE_OFRANGE(name, base, size)        \
        OKL4_BITMAP_ALLOCATOR_DECLARE_OFRANGE(name, base, size)

/**
 *  The OKL4_KSPACE_ID_POOL_DECLARE_OFSIZE() macro is used to declare and 
 *  statically allocate memory for a kspace identifier pool capable of 
 *  allocating a @a size number of values.
 *
 * This function requires the following arguments:
 *
 *  @param name Name of the pool.
 *  @param size Size of the pool.
 *
 *  This macro only handles memory allocation.  The initialization
 *  function must be called separately.
 */
#define OKL4_KSPACE_ID_POOL_DECLARE_OFSIZE(name, size)              \
        OKL4_BITMAP_ALLOCATOR_DECLARE_OFSIZE(name, size)

/* Dynamic initialisers. */

/**
 *  The OKL4_KSPACEID_POOL_SIZE_ATTR() macro is used to determine the amount of 
 *  memory required to dynamically allocate
 *  a kspace identifier pool, if the pool is initialized with the attribute
 *  specified by the @a attr argument, in bytes.
 *
 */
#define OKL4_KSPACEID_POOL_SIZE_ATTR(attr)                          \
        OKL4_BITMAP_ALLOCATOR_SIZE(attr)

/* The Kernel Space Identifier allocator methods */

/**
 * The okl4_kspaceid_pool_init() function is used to initisalize a kspace 
 * allocator with the @a attr attribute. This function is a simple wrapper 
 * around the okl4_bitmap_allocator_init () function.
 *
 *  This function requires the following arguments:
 * 
 *  @param pool Pool to be initialized.
 *  @param attr Attributes with which to initialize.
 *
 */

INLINE void okl4_kspaceid_pool_init(okl4_kspaceid_pool_t *pool,
        okl4_allocator_attr_t *attr);

/**
 *  The okl4_kspaceid_allocany() function is used to allocate any kspace identifier
 *  from the allocator pool specified by the @a pool argument.  The allocated 
 *  identifier is returned using the @a id argument.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    Allocator from which to allocate.
 *
 *  @param id
 *    kspace identifier containing the resultant allocation on function return.
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
int okl4_kspaceid_allocany(okl4_kspaceid_pool_t * pool, okl4_kspaceid_t * id);

/**
 *  The okl4_kspaceid_allocunit() function is used to allocate a specific
 *  kspace identifier from the allocator pool specified by the @a pool argumenet. 
 *  The requested value is encoded in the @a id argument on function return.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    Allocator from which to allocate.
 *
 *  @param id
 *    kspace identifier containing the encoded allocation request. 
 *    This argument is also used to return the  resultant allocation 
 *    on function return.
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
int okl4_kspaceid_allocunit(okl4_kspaceid_pool_t * pool, okl4_kspaceid_t * id);

/**
 *  The okl4_kspaceid_free() function is used to deallocate a space specified 
 *  by the @a id argument from the allocator pool specified by the @a pool
 *  argument.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    Allocator from which to deallocate.
 *
 *  @param id
 *    kspace identifier to be deallocated.
 *
 */
void okl4_kspaceid_free(okl4_kspaceid_pool_t * pool, okl4_kspaceid_t id);

/**
 *  The okl4_kspaceid_getattribute() function is used to retrieve the
 *  allocator attribute that describes the kspace identifier allocator pool, 
 *  specified by the @a pool argument. The result is encoded and returned 
 *  using the @a attr argument.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    Allocator to be queried.
 *
 *  @param attr
 *    Attribute used to contain the returned description. 
 *
 */
void okl4_kspaceid_getattribute(okl4_kspaceid_pool_t * pool, okl4_allocator_attr_t * attr);

/**
 *  The okl4_kspaceid_isallocated() function is used to determine whether a
 *  specified kspace identifier specified by the @a id argument is currently 
 *  allocated by the kspace identifier allocator, @a pool.
 *
 *  This function requires the following arguments:
 *
 *  @param pool
 *    Allocator to be queried.
 *
 *  @param id
 *    kspace identifier to query.
 *
 *  This function returns 1 if the kspace identifier is currently allocated and 
 *  0 if it is free.
 *
 */
int okl4_kspaceid_isallocated(okl4_kspaceid_pool_t * pool, okl4_kspaceid_t id);


INLINE void
okl4_kspaceid_pool_init(okl4_kspaceid_pool_t *pool,
        okl4_allocator_attr_t *attr)
{
    assert(pool != NULL);
    assert(attr != NULL);

    okl4_bitmap_allocator_init(pool, attr);
}

#endif /* !__OKL4__KSPACEID_POOL_H__ */
