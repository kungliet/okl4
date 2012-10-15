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

#ifndef __OKL4__ALLOCATOR_ATTR_H__
#define __OKL4__ALLOCATOR_ATTR_H__

#include <okl4/types.h>
#include <okl4/errno.h>

#include <assert.h>
#include <stddef.h>

/**
 *  @file
 *
 *  The allocator_attr.h Header.
 *
 *  The allocator_attr.h header provides the functionality required for
 *  initializing allocator strategies.  Allocator strategies that are currently 
 *  supported include the bitmap allocator and range allocator. The functionality  
 *  provided by these strategies are described in Chapters bitmap.h and range.h,
 *  respectively.
 *
 *  Allocator strategies are initialized using @a allocator attributes. The
 *  allocator attribute structure is described in Section okl4_allocator_attr.
 *  Allocator attributes are manipulated using accessor functions described
 *  in Sections okl4_allocator_attr_setrange() to okl4_allocator_attr_setsize(),
 *  below.
 *
 */

/**
 *  The OKL4_ALLOC_ANONYMOUS constant is used to represent an anonymous
 *  allocation.
 */
#define OKL4_ALLOC_ANONYMOUS ((okl4_word_t)~0ULL)

/**
 *  The okl4_allocator_attr structure is used to represent an allocator
 *  attribute. This structure is used for initializing allocator
 *  strategies.
 */
struct okl4_allocator_attr
{
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    okl4_word_t base;
    okl4_word_t size;
};

/**
 *  The okl4_allocator_attr_init() function is used to initialize the
 *  attribute specified by the @a attr argument to default values.  
 *  An attribute must be initialized before any 
 *  properties of the attribute can be set.
 *  
 */
INLINE void okl4_allocator_attr_init(okl4_allocator_attr_t *attr);

/**
 *  The okl4_allocator_attr_setrange() function is used to encode an
 *  attribute an attribute specified by the @a attr argument to 
 *  initialize an allocator with the base address and size values specified by the 
 *  @a base and @a size arguments, respectively.  The initialized allocator is now capable
 *  of allocating a @a size number of values starting with @a base.
 *
 *  This function requires the following arguments:
 *
 * @param attr The attribute to encode.
 *
 * @param base The first value available in the allocator.
 *
 * @param size The number of values in the allocator.
 *
 */
INLINE void okl4_allocator_attr_setrange(okl4_allocator_attr_t * attr,
        okl4_word_t base, okl4_word_t size);

/**
 *  The okl4_allocator_attr_setsize() function is used to encode an
 *  attribute specified by the @a attr argument to initialize an allocator with 
 *  a size specified by the @a size argument.  The resulting allocator  
 *  is now capable of allocating a @a size number of values.
 *
 *  This function requires the following arguments:
 *
 *  @param attr The attribute to encode.
 *
 *  @param size The number of values in the allocator.
 *
 */
INLINE void okl4_allocator_attr_setsize(okl4_allocator_attr_t * attr,
        okl4_word_t size);

/*
 *  Inlined methods.
 */

INLINE void
okl4_allocator_attr_init(okl4_allocator_attr_t *attr)
{
    assert(attr != NULL);
    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_ALLOCATOR_ATTR);

    attr->base = 0;
    attr->size = 0;
}

INLINE void
okl4_allocator_attr_setrange(okl4_allocator_attr_t * attr,
        okl4_word_t base, okl4_word_t size)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_ALLOCATOR_ATTR);

    attr->base = base;
    attr->size = size;
}

INLINE void
okl4_allocator_attr_setsize(okl4_allocator_attr_t * attr, okl4_word_t size)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_ALLOCATOR_ATTR);

    attr->base = OKL4_ALLOC_ANONYMOUS;
    attr->size = size;
}

#endif /* !__OKL4__ALLOCATOR_ATTR_H__ */
