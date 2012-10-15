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

#ifndef __OKL4__UTCB_H__
#define __OKL4__UTCB_H__

#include <okl4/bitmap.h>
#include <okl4/types.h>
#include <okl4/virtmem_pool.h>
#include <okl4/range.h>
#include <okl4/utcb_area_attr.h>

/**
 *  @file
 *
 *  The utcb.h Header.
 *
 *  The utcb.h header provides the functionality required for making use of
 *  the libokl4 UTCB interface.  This interface defines the structure
 *  for user thread control blocks (hence the abbreviation of UTCB), and
 *  operations associated with UTCBs. This includes the allocation of @a
 *  utcb items (Section okl4_utcb_item) out of utcb areas (Section
 *  okl4_utcb_area).  The utcb allocator uses the bitmap allocator
 *  (Section bitmap.h) as its underlying allocation strategy.
 *
 *  The user thread control block (@a UTCB) is a thread-specific state that
 *  is readable and writable by the user.  Each thread must have exactly
 *  one UTCB before it starts execution, and no two threads may share the
 *  same UTCB.
 *
 *  The UTCB area is a region of virtual memory, defined for each address
 *  space, that contains all the UTCBs for threads contained in that space.
 *  The size of the UTCB area in a given address space determines how many
 *  maximum threads that address space may support.  The UTCB area also
 *  acts as an allocator for UTCB items.
 *
 */

/**
 *  The okl4_utcb_item struct is used to represent the UTCB object.
 *  This object is used to reference UTCB allocations out of UTCB areas.
 *
 *  The UTCB has the following categories of operations:
 *
 *  @li Allocation (Sections okl4_utcb_allocany() to
 *  okl4_utcb_allocunit());
 *
 *  @li Query (Section okl4_utcb_item_getutcb()); and
 *
 *  @li Free (Section okl4_utcb_free()).
 *
 */
struct okl4_utcb_item {
    okl4_utcb_t *utcb;
};

/**
 *  The okl4_utcb_area struct is used to represent the UTCB area.  A
 *  UTCB area is a region of virtual memory that is dedicated for
 *  containing UTCBs.  The object also acts as an allocator for UTCBs.
 *  Memory for each individual UTCB are allocated out of the UTCB area.
 *
 *  The UTCB area has the following categories of operations:
 *
 *  @li Static memory allocation (OKL4_UTCB_AREA_DECLARE_OFSIZE());
 *
 *  @li Dynamic memory allocation (OKL4_UTCB_AREA_SIZE());
 *
 *  @li Initialization (okl4_utcb_area_init());
 *
 *  @li Destruction (okl4_utcb_area_destroy());
 *
 */
struct okl4_utcb_area {
    okl4_virtmem_item_t utcb_memory;
    okl4_bitmap_allocator_t utcb_allocator; /* variable sized */
};


/* Static Memory Allocators */

/**
 *  The OKL4_UTCB_AREA_SIZE() macro is used to determine the amount of
 *  memory to be dynamically allocated to a UTCB area if it is initialized
 *  with the @a attr attribute.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be used in initialization.
 *
 *  This function returns the amount of memory to allocate, in bytes. The
 *  result of this function is typically passed into a memory allocation
 *  function such as malloc().
 *
 */
/*lint -emacro((433), OKL4_UTCB_AREA_SIZE) */
#define OKL4_UTCB_AREA_SIZE(num_utcbs)                                        \
    ((sizeof(struct okl4_utcb_area) - sizeof(struct okl4_bitmap_allocator)) + \
    OKL4_BITMAP_ALLOCATOR_SIZE(num_utcbs))

/**
 *  The OKL4_UTCB_AREA_DECLARE_OFSIZE() macro is used to declare and
 *  statically allocate memory for a UTCB area with a @a base address and
 *  @a size.  As a result, an @a uninitialized UTCB area is declared with
 *  @a name.
 *
 *  This function requires the following arguments:
 *
 *  @param name
 *    Variable name of UTCB area.
 *
 *  @param base
 *    The base address of UTCB area.
 *
 *  @param size
 *    The size of UTCB area.
 *
 *  This macro only handles memory allocation.  The initialization function
 *  must be called separately.
 *
 */
#define OKL4_UTCB_AREA_DECLARE_OFSIZE(name, base, size)                       \
    okl4_word_t __##name[OKL4_UTCB_AREA_SIZE((size) /                         \
            sizeof(okl4_utcb_t))];                                            \
    static okl4_utcb_area_t *(name) = (okl4_utcb_area_t *)(void *) __##name

/* Dynamic Memory Allocators */

/**
 *  Determine the amount of memory to be dynamically allocated to
 *  a utcb area, if the utcb area is initialised with the
 *  @a attr attribute.
 *
 *  @param attr The attribute to be used in initialisation.
 *  @return The amount of memory to allocate, in bytes.
 *
 *  The result of this function is typically passed into a memory
 *  allocation function such as malloc().
 */
#define OKL4_UTCB_AREA_SIZE_ATTR(attr) \
    OKL4_UTCB_AREA_SIZE((attr)->utcb_range.size / sizeof(okl4_utcb_t))

/* API Interface */

/**
 *  The okl4_utcb_area_init() function is used to initialise the UTCB area
 *  object @a ms with the @a attr attribute.
 *
 *  This function requires the following arguments:
 *
 *  @param utcb_area
 *    The UTCB area to initialize.
 *
 *  @param attr
 *    The attribute to initialize with.
 *
 */
void okl4_utcb_area_init(okl4_utcb_area_t *utcb_area,
        okl4_utcb_area_attr_t *attr);

/**
 *  The okl4_utcb_area_destroy() function is used to destroy a @a
 *  utcb_area.  A destroyed UTCB area may no longer be used. The memory it
 *  used to occupy may be freed or used for other purposes.
 *
 *  This function requires the following arguments:
 *
 *  @param utcb_area
 *    The UTCB area to destroy.
 *
 */
void okl4_utcb_area_destroy(okl4_utcb_area_t *utcb_area);

/**
 *  The okl4_utcb_allocany() function is used to allocate any UTCB from a
 *  @a utcb_area.  The allocation is returned in @a utcb.  The @a utcb
 *  object encodes both the base address and size of the allocated UTCB.
 *
 *  This function requires the following arguments:
 *
 *  @param utcb_area
 *    The UTCB area to allocate from.
 *
 *  @param utcb
 *    The UTCB item that contains the resultant allocation after the
 *    function returns.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    The allocation is successful.
 *
 *  @param others
 *    The return value from okl4_bitmap_allocator_alloc().
 *
 */
int okl4_utcb_allocany(okl4_utcb_area_t *utcb_area,
        okl4_utcb_item_t *utcb);

/**
 *  The okl4_utcb_allocunit() function is used to allocate a specific UTCB
 *  (@a unit) from @a utcb_area. The allocation is returned in @a utcb.
 *  The @a utcb object encodes both the base address and size of the
 *  allocated UTCB.
 *
 *  This function requires the following arguments:
 *
 *  @param utcb_area
 *    The UTCB area to allocate from.
 *
 *  @param utcb
 *    The UTCB that contains the resultant allocation after the function
 *    returns.
 *
 *  @param unit
 *    The UTCB number that is requested.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    The allocation is successful.
 *
 *  @param others
 *    The return value from okl4_bitmap_allocator_alloc().
 *
 */
int okl4_utcb_allocunit(okl4_utcb_area_t *utcb_area,
        okl4_utcb_item_t *utcb, okl4_word_t unit);

/**
 *  The okl4_utcb_free() function is used to deallocate a @a utcb from @a
 *  utcb_area.
 *
 *  This function requires the following arguments:
 *
 *  @param utcb_area
 *    The UTCB area to deallocate back to.
 *
 *  @param utcb
 *    The UTCB to free.
 *
 */
void okl4_utcb_free(okl4_utcb_area_t *utcb_area,
        okl4_utcb_item_t *utcb);

/**
 *  The okl4_utcb_item_getutcb() function is used to retrieve the @a utcb
 *  within the UTCB @a item object.
 *
 *  This function requires the following arguments:
 *
 *  @param item
 *    The UTCB item containing the utcb.
 *
 *  @param utcb
 *    The encoded UTCB object.  Its contents will contain the UTCB encoded
 *    in @a item when the function returns.
 *
 */
INLINE void
okl4_utcb_item_getutcb(okl4_utcb_item_t *item, okl4_utcb_t **utcb);

/*
 * Inlined methods
 */

INLINE void
okl4_utcb_item_getutcb(okl4_utcb_item_t *item, okl4_utcb_t **utcb)
{
    *utcb = item->utcb;
}


#endif /* ! __OKL4__UTCB_H__ */
