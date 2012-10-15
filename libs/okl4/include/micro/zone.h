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

#ifndef __OKL4__ZONE_H__
#define __OKL4__ZONE_H__

#include <assert.h>
#include <okl4/types.h>
#include <okl4/memsec.h>
#include <okl4/pd.h>

#include "mem_container.h"

/**
 * Minimum alignment for zone virtual addresses and zone sizes.
 */
#ifdef ARM_SHARED_DOMAINS
#define OKL4_ZONE_ALIGNMENT 0x100000

/* On ARM9 a zone can have one of two different permission
 * configurations. The first option below requires that all
 * memsections attached to a zone have the same permissions -- and in
 * this configuration the attach permissions for the zone can vary
 * between attachments to PDs. The second option is that the attached
 * memsections can have any permissions, but the attach permissions
 * for the zone must always be rwx (i.e. L4_FullyAccessible). */
#define OKL4_ZONE_MEMSEC_PERMS_ONE  0
#define OKL4_ZONE_MEMSEC_PERMS_ANY  1
#else
#define OKL4_ZONE_ALIGNMENT OKL4_DEFAULT_PAGESIZE
#endif


/**
 *  @file
 *
 *  The zone.h Header.
 *
 *  The zone.h header provides the functionality required for making
 *  use the zone interface.
 *
 *  Zones are a mechanism for sharing virtual-to-physical memory mappings,
 *  or more precisely, hardware MMU contexts.  Zones are utilized by first
 *  attaching to one or more protection domains (okl4_pd_pd()). Memsections
 *  (okl4_memsec_memsec()), in turn, are attached to zones. PDs that are
 *  attached to a given zone have access to memsections contained in that
 *  zone.
 *
 */

#define OKL4_ZONE_DEFAULT_MAX_PDS  2

/**
 *  The struct okl4_zone struct is used to represent the zone object.  The
 *  zone is a mechanism for sharing virtual-to-physical memory mappings.
 *  Memsections are attached to zones, zones are then attached to
 *  protection domains.
 *
 *  This header contains the following functionality related to the zone
 *  object:
 *
 *  @li Dynamic memory allocation (okl4_zone_size());
 *
 *  @li Creation (okl4_zone_create());
 *
 *  @li Deletion (okl4_zone_delete()); and
 *
 *  @li Memsection Attachment and Detachment (Sections okl4_zone_attach()
 *  and okl4_zone_detach()).
 *
 *  The functions for attaching zones to protection domains are described
 *  in the okl4/pd.h header (pd.h).
 *
 */
struct okl4_zone
{
    /* This element is a generic memory container. It represents the 'super
     * class' of a struct okl4_zone. So we can say struct okl4_zone 'is a'
     * struct _okl4_mem_container. */
    _okl4_mem_container_t super;

    /* List of memory container nodes representing the memsections attached to
     * this zone, sorted by increasing virtual base address. */
    _okl4_mcnode_t *mem_container_list;

    /* A pointer to a chunk of memory reserved at zone creation time. This
     * memory is used by the pds to maintain a list of all attached zones. */
    _okl4_mcnode_t *mcnode_pool;

    /* Bitmap allocator to keep track of which nodes in the above memory
     * container nodes have been used. */
    okl4_bitmap_allocator_t *mcnode_alloc;

#if defined(ARM_SHARED_DOMAINS)
    /* kspace id for this zone. */
    okl4_kspaceid_t kspaceid;

    /* Pool of kspace identifiers, used to get a kspace id for the
     * zone if none is provided by the user. */
    okl4_kspaceid_pool_t *kspaceid_pool;
#endif

    /* Maximum number of PDs we can attach to. */
    okl4_word_t max_pds;

    /* Number of PDs we are attached to. */
    okl4_word_t num_parent_pds;

    /* A pointer to a chunk of memory reserved at zone creation time. This
     * memory is used by the zone to maintain a list of all attached pds. */
    okl4_pd_t **parent_pds;
};

/*
 * Static Declaration Macros
 */

/**
 *  No comment.
 */
#define _OKL4_ZONE_MCNODE_POOL_SIZE(num_pds)                            \
    (sizeof(_okl4_mcnode_t) * (num_pds))

/**
 *  The OKL4_ZONE_SIZE() macro is used to determine the amount of memory to
 *  be dynamically allocated to a zone if it is created with the @a attr
 *  attribute.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to be used in creation.
 *
 *  This function returns the amount of memory to allocate, in bytes. The
 *  result of this function is typically passed into a memory allocation
 *  function such as malloc().
 *
 */
#define OKL4_ZONE_SIZE(max_pds)                                         \
    (sizeof(struct okl4_zone)                                           \
            + _OKL4_ZONE_MCNODE_POOL_SIZE(max_pds)                      \
            + OKL4_BITMAP_ALLOCATOR_SIZE(max_pds)                       \
            + (sizeof(okl4_pd_t *) * max_pds))

/**
 *  Declare and statically allocate memory for a
 *  zone structure.
 *
 *  @param name
 *      Variable name of the zone.
 *
 *  @param max_pd
 *      The maximum pds a zone can attach to.
 *
 */
#define OKL4_ZONE_DECLARE(name, max_pds)                                \
    okl4_word_t __##name[OKL4_ZONE_SIZE(max_pds)];                      \
    static struct okl4_zone *(name)  = (okl4_zone_t *) __##name

/*
 * Dynamic Initializer.
 */

/**
 *  Determine the amount of memory to be dynamically allocated to
 *  the zone structure, if it is initialized with the @a attr attribute
 *
 *  @param attr
 *      The attribute to be used in initialization.
 *
 *  @return
 *      The amount of memory to allocate, in bytes.
 *
 *  The result of this functionis typically passed into a memory
 *  allocation function such as malloc()
 */
#define OKL4_ZONE_SIZE_ATTR(attr)                                       \
    OKL4_ZONE_SIZE((attr)->max_pds)

/*
 * Public zone object functions.
 */

/**
 *  The okl4_zone_create() function is used to create the @a zone object
 *  with the @a attr attribute.
 *
 *  Although the current implementation of okl4_zone_create() does not
 *  involve the creation of any new objects, only initialization; it is
 *  nonetheless labelled as a @a creation operation due to the reasoning
 *  described in okl4_zone_attr().
 *
 *  This function requires the following arguments:
 *
 *  @param zone
 *    The zone to create.  Its contents will contain the newly created zone
 *    after the function returns.
 *
 *  @param attr
 *    The attribute to create with.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    The creation is successful.
 *
 *  Although the current implementation of okl4_zone_create() always return
 *  a success code, future versions will have the option of failing under
 *  certain circumstances.  Users of this API function should check for the
 *  error code returned by this function.
 *
 */
int okl4_zone_create(okl4_zone_t *zone, okl4_zone_attr_t *attr);

/**
 *  The okl4_zone_delete() function is used to delete a @a zone object.
 *  The zone must not have any memsections attached to it, and it must not
 *  be attached to any protection domains.
 *
 *  This function requires the following arguments:
 *
 *  @param zone
 *    The zone to delete.
 *
 */
void okl4_zone_delete(okl4_zone_t *zone);

/**
 *  The okl4_zone_memsec_attach() function is used to attach a memsection
 *  @a memsec to a @a zone.  This memsection will be shared by all
 *  protections domains that @a zone is attached to.  The virtual memory
 *  region that @a memsec is located at must be entirely contained in the
 *  virtual memory region occupied by @a zone.  In addition, @a memsec must
 *  not overlap with any other memsections already attached to @a zone.
 *
 *  This function requires the following arguments:
 *
 *  @param zone
 *    The zone to attach the memsection to.
 *
 *  @param memsec
 *    The memsection to attach.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    The attachment is successful.
 *
 *  @param OKL4_ALLOC_EXHAUSTED
 *    The @a memsec falls outside of the @a zone's virtual memory region,
 *    or @a memsec overlaps with another memsection already attached to @a
 *    zone.
 *
 */
int okl4_zone_memsec_attach(okl4_zone_t *zone, okl4_memsec_t *memsec);

/**
 *  The okl4_zone_memsec_detach() function is used to detach a memsection
 *  @a memsec from a @a zone.  This memsection will cease to be shared by
 *  protection domains attached to @a zone.
 *
 *  This function requires the following arguments:
 *
 *  @param zone
 *    The zone to detach the memsection from.
 *
 *  @param memsec
 *    The memsection to detach.
 *
 */
void okl4_zone_memsec_detach(okl4_zone_t *zone, okl4_memsec_t *memsec);

/**
 *  Perform a map operation on this zone, this will be passed down to the
 *  appropriate attached memsection in to determining where the given 'vaddr' should
 *  map to in physical memory.
 */
int _okl4_zone_access(okl4_zone_t *zone, okl4_word_t vaddr, okl4_physmem_item_t
        *phys, okl4_word_t *dest_vaddr, okl4_word_t *page_size, okl4_word_t
        *perms, okl4_word_t *attributes);

/**
 *  The okl4_zone_attr struct is used to represent the zone
 *  attribute.  This structure is used to create zones.
 *
 *  The following steps need to be performed on a zone attribute before it
 *  may be used to create zones:
 *
 *  @li [a{)}] The attribute must be initialized (okl4_zone_ainit()),
 *
 *  @li [b{)}] The virtual memory region that the zone covers
 *  (okl4_zone_setrange()), and
 *
 *  @li [c{)}] The maximum number of PDs that the zone can attach to
 *  (okl4_zone_setmaxpds()).
 *
 *  It should be noted that the correct term for constructing a zone is
 *  indeed @a creation as opposed to @a initialization.  This is because in
 *  future implementations, it is expected that zone creation may consume
 *  some amount of kernel resources.  Constructing a new zone in libokl4 is
 *  expected to invoke kernel system calls in the future.  This is
 *  reflected in the naming of API functions, and also in this document.
 *
 */
struct okl4_zone_attr
{
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif

#if defined(ARM_SHARED_DOMAINS)
    /* The kspace associated with this zone. */
    okl4_kspaceid_t kspaceid;

    /* Pool of kspace identifiers - needed to create the space of this
     * zone if one is not passed in. */
    okl4_kspaceid_pool_t *kspaceid_pool;
#endif

    /* The range of virtual addresses covered by the zone. */
    okl4_virtmem_item_t range;

    /* The maximum number of pd's that this zone can attach to. */
    okl4_word_t max_pds;
};

/**
 *  The okl4_zone_attr_init() function is used to initialize the zone
 *  attribute @a attr to default values. It must be invoked on the
 *  attribute before it may be used for zone creation.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to encode. Its contents will contain default values
 *    after the function returns.
 *
 */
INLINE void okl4_zone_attr_init(okl4_zone_attr_t *attr);

/**
 *  The okl4_zone_attr_setrange() function is used to encode a virtual
 *  memory region @a range in the zone attribute @a attr.  This specifies
 *  the virtual memory region that the zone is located at.  When a zone is
 *  attached to a PD, this will be the region that it is attached to;
 *  memsections that are attached to this zone must be entirely contained
 *  within the virtual memory region of the zone.
 *
 *  When attaching this zone to a PD, its virtual memory region must not
 *  overlap with any other memsections, zones, or extensions already
 *  attached to the PD.
 *
 *  The virtmem item, @a range, is constructed using functions described in
 *  vmempool.h.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to encode. Its contents will reflect the new encodoing
 *    after the function returns.
 *
 *  @param range
 *    The virtual memory region of the zone.
 *
 */
INLINE void okl4_zone_attr_setrange(okl4_zone_attr_t *attr,
        okl4_virtmem_item_t range);

/**
 *  The okl4_zone_attr_setmaxpds() function is used to encode the maximum
 *  number of protection domains that a given zone is able to be attached
 *  to.  After the zone is created, this maximum will be fixed; it cannot
 *  be increased after creation.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to encode. Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param max_pds
 *    The maximum number of PDs that the zone can be attached to.
 *
 */
INLINE void okl4_zone_attr_setmaxpds(okl4_zone_attr_t *attr, okl4_word_t max_pds);

/*
 * Inline functions.
 */

INLINE void
okl4_zone_attr_init(okl4_zone_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_ZONE_ATTR);
#if defined(ARM_SHARED_DOMAINS)
    attr->kspaceid.raw = ~0UL;
    attr->kspaceid_pool = NULL;
#endif
    attr->range.base = 0;
    attr->range.size = 0;
    attr->max_pds = OKL4_ZONE_DEFAULT_MAX_PDS;
}

#if defined(ARM_SHARED_DOMAINS)
INLINE void
okl4_zone_attr_setkspaceid(okl4_zone_attr_t *attr, okl4_kspaceid_t kspaceid)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_ZONE_ATTR);

    attr->kspaceid = kspaceid;
}

INLINE void
okl4_zone_attr_setkspaceidpool(okl4_pd_attr_t *attr,
        okl4_kspaceid_pool_t *kspaceid_pool)
{
    assert(attr != NULL);
    /* Do not assert kspaceid_pool != NULL, this is a valid input. */
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_PD_ATTR);

    attr->kspaceid_pool = kspaceid_pool;
}
#endif

INLINE void
okl4_zone_attr_setrange(okl4_zone_attr_t *attr, okl4_virtmem_item_t range)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_ZONE_ATTR);

    attr->range = range;
}

INLINE void
okl4_zone_attr_setmaxpds(okl4_zone_attr_t *attr, okl4_word_t max_pds)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_ZONE_ATTR);

    attr->max_pds = max_pds;
}


#endif /* !__OKL4__ZONE_H__ */
