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

#ifndef __OKL4__STATIC_MEMSEC_H__
#define __OKL4__STATIC_MEMSEC_H__

#include <assert.h>

#include <okl4/types.h>
#include <okl4/virtmem_pool.h>
#include <okl4/physmem_item.h>
#include <okl4/memsec.h>

/**
 *  @file
 *
 *  The static_memsec.h Header.
 *
 *  The static_memsec.h header provides the functionality required for
 *  making use of the static memsection allocator interface.
 *
 *  The static memsection is a special case of the memsection object
 *  (okl4_memsec_memsec()).  A static memsection also represents a virtual
 *  memory region and its mapping in physical memory.  Unlike the general
 *  memsection, the static memsection has its mappings fully resolved at
 *  the time of initialization, and the mapping cannot be changed after.
 *  The virtual-to-physical mapping is always contiguous and not
 *  segregated.
 *
 *
 */

/*
 * Static Declaration macros
 */

/**
 *  The OKL4_STATIC_MEMSEC_SIZE() macro is used to determine the amount of
 *  memory to be dynamically allocated to a static memsection if the
 *  memsection is initialized with the @a attr attribute.
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
#define OKL4_STATIC_MEMSEC_SIZE      \
    (sizeof(struct okl4_static_memsec))

/**
 *  No comment.
 */
#define OKL4_STATIC_MEMSEC_DECLARE(name)            \
    okl4_static_memsec_t __##name;                  \
    static okl4_static_memsec_t *(name) = &__##name

/**
 *  Return the amount of memory required for an okl4_static_memsec_t to
 *  be created with the given attributes object.
 */
#define OKL4_STATIC_MEMSEC_SIZE_ATTR(attr) \
    (sizeof(struct okl4_static_memsec))

/**
 *  The okl4_static_memsec struct is used to represent the static
 *  memsection object.  The memsection object contains a virtual memory
 *  region and its immutable mapping to physical memory.
 *
 *  The static memsection has the following categories of operations:
 *
 *  @li Dynamic memory allocation (okl4_sms_size());
 *
 *  @li Initialization (okl4_sms_init()); and
 *
 *  @li Query (okl4_sms_getms()).
 *
 */
struct okl4_static_memsec {
    /** Base okl4_memsec_t object. */
    okl4_memsec_t base;

    /** Target physical memory item that we map to. */
    okl4_physmem_item_t target;
};

/**
 *  The okl4_static_memsec_init() function is used to initialise the static
 *  memsection object @a ms with the @a attr attribute.
 *
 *  This function requires the following arguments:
 *
 *  @param ms
 *    The static memsection to initialize.
 *
 *  @param attr
 *    The attribute to initialize with.
 *
 */
void okl4_static_memsec_init(okl4_static_memsec_t *ms,
        okl4_static_memsec_attr_t *attr);

/**
 *  The okl4_static_memsec_getmemsec() function is used to retrieve the
 *  memsection contained within the static memsection @a ms.
 *
 *  @param ms
 *    The static memsection to retrieve from.
 *
 *  This function returns a memsection object that is equivalent to the
 *  static memsection @a ms.
 *
 */
INLINE okl4_memsec_t * okl4_static_memsec_getmemsec(okl4_static_memsec_t *ms);

/**
 *  The struct okl4_static_memsec_attr struct is used to represent the
 *  static memsection attribute.  This structure is used to initialize
 *  static memsections.
 *
 *  The following parameters are required when initializing a static
 *  memsection:
 *
 *  @li The virtual memory region represented by the memsection
 *  (okl4_sms_setrange() or okl4_sms_setseg());
 *
 *  @li The physical memory region mapped to the memsection
 *  (okl4_sms_settarget());
 *
 *  @li The secondary attributes associated with mapping the memsection,
 *  including its page size (\Autoref{sec:sms.setpagesize}), access
 *  permissions (okl4_sms_setperms()) and cache attributes
 *  (okl4_sms_setattr());
 *
 *  Each of these parameters should be set appropriately in the static
 *  memsection attribute before initializing the memsection.  In addition,
 *  there is an initialization function defined for the memsection
 *  attribute (okl4_sms_ainit()).  The static memsection attribute must be
 *  initialized before it is used to initialize static memsections.
 *
 */
struct okl4_static_memsec_attr {
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif

    /** Segment - create a memsection representing this segment */
    okl4_env_segment_t *segment;

    /** Page size to use when mapping */
    okl4_word_t page_size;

    /** Memory permissions. */
    okl4_word_t perms;

    /** Memory attributes. */
    okl4_word_t attributes;

    /** Source virtual memory address range this memsec should use. */
    okl4_virtmem_item_t range;

    /** Target physical memory range this memsec should map to. This must be at
     * least as large as the virtual range. */
    okl4_physmem_item_t target;
};

/**
 *  The okl4_static_memsec_attr_init() function is used to initialize a
 *  static memsection attribute @a attr.  It must be invoked on the
 *  attribute before it may be used for static memsection initialization.
 *
 *  @param attr
 *    The attribute to initialize. Its contents will contain default values
 *    after the function returns.
 *
 */
void okl4_static_memsec_attr_init(okl4_static_memsec_attr_t *attr);

/**
 *  The okl4_static_memsec_attr_setsegment() function is used to encode the
 *  static memsection attribute @a attr with the virtual memory region
 *  contained in an environment @a segment.  The static memsection
 *  initialized with this attribute will be used to represent the virtual
 *  memory mapping of the @a segment.
 *
 *  An environment segment represents an ELF executable segment.
 *  Therefore, initializing a static memsection with segments is
 *  particularly useful for mapping executable code sections.
 *
 *  @param attr
 *    The attribute to encode. Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param segment
 *    The environment segment containing the virtual memory region.
 *
 */
INLINE void okl4_static_memsec_attr_setsegment(okl4_static_memsec_attr_t *attr,
        okl4_env_segment_t *segment);

/**
 *  The okl4_static_memsec_attr_setrange() function is used to encode the
 *  static memsection attribute @a attr with the virtual memory region @a
 *  range.  The static memsection initialized with this attribute will be
 *  used to represent the virtual memory mapping of the @a range region.
 *  The virtmem item, @a range, is constructed with functions defined in
 *  vmempool.h.
 *
 *  @param attr
 *    The attribute to encode. Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param range
 *    The virtmem item containing the virtual memory region.
 *
 */
INLINE void okl4_static_memsec_attr_setrange(okl4_static_memsec_attr_t *attr,
        okl4_virtmem_item_t range);

/**
 *  The okl4_memsec_attr_setpagesize() function is used to encode the
 *  memsection attribute @a attr with a page size. This controls the
 *  granularity of mappings within the static memsection.
 *
 *  @param attr
 *    The attribute to encode.  Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param page_size
 *    The granularity of mappings in the memsection.
 *
 */
INLINE void okl4_static_memsec_attr_setpagesize(okl4_static_memsec_attr_t *attr,
        okl4_word_t page_size);

/**
 *  The okl4_static_memsec_attr_setperms() function is used to encode the
 *  static memsection attribute @a attr with access permissions. These
 *  access permissions are used to control access to the
 *  virtual-to-physical mappings contained within the static memsection.
 *
 *  @param attr
 *    The attribute to encode.  Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param perms
 *    The access permissions permitted on the memsection.
 *
 */
INLINE void okl4_static_memsec_attr_setperms(okl4_static_memsec_attr_t *attr,
        okl4_word_t perms);

/**
 *  The okl4_static_memsec_attr_setattributes() function is used to encode
 *  the static memsection attribute @a attr with cache attributes.  This
 *  parameter determines how the physical memory mapped to the memsection
 *  will be cached.
 *
 *  @param attr
 *    The attribute to encode.  Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param attributes
 *    The cache behaviour of physical memory mapped to this memsection.
 *
 */
INLINE void okl4_static_memsec_attr_setattributes(okl4_static_memsec_attr_t *attr,
        okl4_word_t attributes);

/**
 *  The okl4_static_memsec_attr_settarget() function is used to encode the
 *  static memsection attribute @a attr with the physical memory region @a
 *  target.  The static memsection initialized with this attribute will be
 *  mapped to the physical memory region @a target. The physmem item, @a
 *  target, is constructed with functions defined in pmemitem.h.
 *
 *  @param attr
 *    The attribute to encode. Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param target
 *    The physmem item containing the physical memory region to map to.
 *
 */
INLINE void okl4_static_memsec_attr_settarget(okl4_static_memsec_attr_t *attr,
        okl4_physmem_item_t target);

/*
 * Inline functions.
 */

INLINE void
okl4_static_memsec_attr_setsegment(okl4_static_memsec_attr_t *attr,
        okl4_env_segment_t *segment)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_STATIC_MEMSEC_ATTR);

    attr->segment = segment;
}

INLINE void
okl4_static_memsec_attr_setpagesize(okl4_static_memsec_attr_t *attr,
        okl4_word_t page_size)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_STATIC_MEMSEC_ATTR);

    attr->page_size = page_size;
}

INLINE void
okl4_static_memsec_attr_setperms(okl4_static_memsec_attr_t *attr,
        okl4_word_t perms)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_STATIC_MEMSEC_ATTR);

    attr->perms = perms;
}

INLINE void
okl4_static_memsec_attr_setattributes(okl4_static_memsec_attr_t *attr,
        okl4_word_t attributes)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_STATIC_MEMSEC_ATTR);

    attr->attributes = attributes;
}

INLINE void
okl4_static_memsec_attr_setrange(okl4_static_memsec_attr_t *attr,
        okl4_virtmem_item_t range)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_STATIC_MEMSEC_ATTR);

    attr->range = range;
}

INLINE void
okl4_static_memsec_attr_settarget(okl4_static_memsec_attr_t *attr,
        okl4_physmem_item_t target)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_STATIC_MEMSEC_ATTR);

    attr->target = target;
}

INLINE okl4_memsec_t *
okl4_static_memsec_getmemsec(okl4_static_memsec_t *ms)
{
    return &ms->base;
}


#endif /* !__OKL4__STATIC_MEMSEC_H__ */
