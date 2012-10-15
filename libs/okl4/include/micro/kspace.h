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

#ifndef __OKL4__KSPACE_H__
#define __OKL4__KSPACE_H__

#include <okl4/arch/config.h>

#include <l4/types.h>
#include <l4/misc.h>

#include <okl4/kspaceid_pool.h>
#include <okl4/utcb.h>
#include <okl4/kclist.h>
#include <okl4/kthread.h>
#include <okl4/physmem_item.h>

/**
 *  @file
 *
 *  The kspace.h Header.
 *
 *  The kspace.h header provides the functionality required for
 *  using the kernel address space or @a kspace interface and its associated
 *  components. The kspace interface provides a layer of abstraction between cell managers
 *  and the kernel interface for address spaces. 
 *
 */

/**
 *  The okl4_kspace_attr structure represents the kspace
 *  attribute used to create kspaces.
 *
 *  As the kspace is not a purely user-level construct, constructing a new 
 *  kspace using the OKL4 library always invokes a kernel system call to 
 *  create a space within the kernel.
 *
 *  A valid kspace id must be assigned to a new kspace.  As each cell may only
 *  maintain a fixed quota of kspaces, so the kspace identifier
 *  is used to keep track of address space usage in the kernel.  Valid
 *  kspace identifiers are obtained from the kspace identifier allocator.
 *
 */
struct okl4_kspace_attr {
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    okl4_kspaceid_t id;
    okl4_kclist_t *kclist;
    okl4_utcb_area_t *utcb_area;
    okl4_word_t privileged;
};

/**
 *  The okl4_kspace structure is used to represent a kernel address space
 *  as well as maintaining a list of kthreads attached to the kspace.
 *
 *  The following operations may be performed on a kspace:
 *
 *  @li Static memory allocation.
 *
 *  @li Dynamic memory allocation.
 *
 *  @li Creation.
 *
 *  @li Deletion.
 *
 */
struct okl4_kspace {
    okl4_kspaceid_t id;
    okl4_kclist_t *kclist;
    okl4_utcb_area_t *utcb_area;
    okl4_kthread_t *kthread_list;
    okl4_kspace_t *next;
    okl4_word_t privileged;
};

/**
 *  The okl4_kspace_map_attr structure is used to represent a kspace
 *  map attribute.  This structure is used to encode the parameters 
 *  required to perform a @a map operation.
 *
 *  Note that the map attribute must be initialized using the function, 
 *  okl4_kspace_mainit () prior to being used in a map operation.
 *
 *  The kspace map attribute encodes the following information:
 *
 *  @li The @a range or region of virtual memory to be mapped.
 *
 *  @li The @a target or the region of physical memory to be mapped.
 *
 *  @li The cache @a attributes to be used in the mapping operation.
 *
 *  @li The @a permissions to be used in the memory mapping.
 *
 *  @li The @a page size to be used as the granularity of mapping.
 *
 *  The kspace map attribute initialization function encodes the default
 *  values for each parameter with the exception of the virtual
 *  and physical memory regions.
 *
 */
struct okl4_kspace_map_attr {
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif

    /* Source virtual address of the mapping. */
    okl4_virtmem_item_t *range;

    /* Destination physical address of the mapping. */
    okl4_physmem_item_t *target;

    /* Memory attributes to use in the mapping. */
    okl4_word_t attributes;

    /* Permissions used to in the mapping. */
    okl4_word_t perms;

    /* Page size used to perform the mapping. */
    okl4_word_t page_size;
};

/**
 *  The okl4_kspace_unmap_attr structure represents a @a
 *  kspace unmap attribute used to encode the parameters required to 
 *  perform an @a unmap operation.
 *
 *  Note that the unmap attribute must be initialized prior to being 
 *  used in an unmap operation.
 *
 *  The kspace unmap attribute encodes the following information:
 *
 *  @li The @a range or region of virtual memory to be unmapped.
 *
 *  @li The @a page size to be used as the granularity of unmapping.
 *
 *  The kspace unmap attribute must always be initialized before it may be
 *  used, failure to do so may produce undefined results.
 *
 */
struct okl4_kspace_unmap_attr {
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif

    /* Virtual address range to unmap. */
    okl4_virtmem_item_t *range;

    /* Page size used to perform the unmap operation. This must match the page
     * size used to initially map it in. */
    okl4_word_t page_size;
};

/* Static Memory Allocation */

/**
 *  The OKL4_KSPACE_SIZE() macro is used to determine the amount of memory
 *  required to dynamically allocate a kspace, if the kspace is initialized
 *  with the attribute specified by the @a attr argument, in bytes.
 *
 */
#define OKL4_KSPACE_SIZE       \
    (sizeof(struct okl4_kspace))

/**
 *  The OKL4_KSPACE_DECLARE() macro is used to declare and statically
 *  allocate memory for a kspace. The resulting @a uninitialized kspace
 *  with an appropriate amount of memory will be returned using the 
 *  @a name argument.
 *
 *  This macro only handles memory allocation. The initialization function
 *  must be called separately.
 *
 */
#define OKL4_KSPACE_DECLARE(name)                             \
   okl4_word_t __##name[OKL4_KSPACE_SIZE];               \
   static okl4_kspace_t *(name) = (okl4_kspace_t *)(void *)__##name

/* Dynamic Memory Allocation */

/**
 *  The  OKL4_KSPACE_SIZE_ATTR() macro is used to determine the amount of 
 *  memory required to dynamically allocate a kspace, if the kspace is 
 *  initialized with the attribute specified by the @a attr argument, 
 *  in bytes.
 *
 */
#define OKL4_KSPACE_SIZE_ATTR(attr)  sizeof(struct okl4_kspace)

/* API Interface */

/**
 *  The okl4_kspace_create() function is used to create a @a kspace with
 *  the @a attr attribute. 
 *
 *  This function expects the specified attribute to contain a valid kspace
 *  identifier, a valid kclist for the kspace to attach to and a valid utcb area
 *  in virtual memory.  This function invokes the L4_SpaceControl() system
 *  call to create a kernel space with parameters encoded in the @a attr
 *  attribute. 
 *
 *  This function requires the following arguments:
 *
 *  @param kspace
 *    This argument is used to return the newly created kspace. 
 *
 *  @param attr
 *    Attributes of the kspace to be created.
 *
 *  This function returns the following error codes:
 *
 *  @param OKL4_OK
 *    Creation was successful.
 *
 *  @param other
 *    Return value from L4_SpaceControl().
 *
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation
 *  of the L4_SpaceControl() system call.
 *
 */
int okl4_kspace_create(okl4_kspace_t *kspace,
        okl4_kspace_attr_t *attr);

/**
 *  The okl4_kspace_delete() function is used to delete a kspace specified by 
 *  the @a kspace argument.  This
 *  function invokes the L4_SpaceControl() system call to delete the kernel
 *  address space.  Note that the specified kspace cannot have any attached kthreads.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Deletion was successful.
 *
 *  @param OKL4_IN_USE
 *    There are one or more threads attached to the space.
 *
 *  @param other
 *   Return value from L4_SpaceControl().
 *
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation
 *  of the L4_SpaceControl() system call.
 *
 */
void okl4_kspace_delete(okl4_kspace_t *kspace);

/**
 *  The okl4_kspace_attr_setkclist() function is used to encode a kspace
 *  attribute @a attr with a kclist @a kclist. Creating a kspace with this attribute
 *  will result in it being attached to @a kclist when the kspace is
 *  created.  That means @a kspace will have access to the capabilities contained
 *  in @a kclist.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded. 
 *
 *  @param kclist
 *    kclist to be encoded.
 *
 */
INLINE void okl4_kspace_attr_setkclist(okl4_kspace_attr_t *attr,
        okl4_kclist_t *kclist);

/**
 *  The okl4_kspace_attr_setid() function is used to encode a kspace
 *  attribute @a attr with a @a kspaceid. Creating a kspace with this
 *  attribute will result in that space being identified by @a kspaceid, and
 *  consumes the @a kspaceid slot in the kernel if it is available to the
 *  calling cell.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.  
 *
 *  @param kspaceid
 *    kspace identifier to be encoded.
 *
 */
INLINE void okl4_kspace_attr_setid(okl4_kspace_attr_t *attr,
        okl4_kspaceid_t kspaceid);

/**
 *  The okl4_kspace_attr_setutcbarea() function is used to encode a kspace
 *  attribute @a attr with @a utcb_area. Creating a kspace with this
 *  attribute will result in it reserving a @a utcb_area for the exclusive
 *  use storing utcb objects. The @a utcb_area object is manipulated
 *  using functions described in the okl4/utcb.h header.  The size
 *  of this area determines the number of threads supported
 *  by the kspace. The @a utcb_area is expressed as a region of virtual memory.
 *  
 *  @param attr
 *    Attribute to be encoded. 
 *
 *  @param utcb_area
 *    Utcb area to be encoded.
 *
 */
INLINE void okl4_kspace_attr_setutcbarea(okl4_kspace_attr_t *attr,
        okl4_utcb_area_t *utcb_area);

/**
 *  The okl4_kspace_attr_init() function is used to initialize the kspace
 *  attribute @a attr with default values.
 *
 */
INLINE void okl4_kspace_attr_init(okl4_kspace_attr_t *attr);

/**
 *  The okl4_kspace_map() function is used to perform a virtual-to-physical
 *  mapping operation on the kspace specified by the @a kspace argument, 
 *  using the parameters encoded in the @a attr argument. 
 *  This function invokes the L4_ProcessMapItem() kernel system call
 *  to perform the mapping.
 *
 *  This function requires the following arguments:
 *
 *  @param kspace
 *    kspace on which the mapping is performed.
 *
 *  @param attr
 *    Attribute encoding mapping parameters.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Mapping was successful.
 *
 *  @param others
 *    Return value from L4_ProcessMapItem().
 *
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation
 *  of the L4_ProcessMapItem() system call.
 *
 */
int okl4_kspace_map(okl4_kspace_t *kspace, okl4_kspace_map_attr_t *attr);

/*
 *  Internal function to map a virtual address range into the kspace
 *  with the given id.
 *
 *  @param[in] kspaceid  The kspaceid to perform the mapping on.
 *  @param[in] attr  Attributes containing information about the map to
 *  perform.
 */
int _okl4_kspace_mapbyid(okl4_kspaceid_t kspaceid,
        okl4_kspace_map_attr_t *attr);

/**
 *  The okl4_kspace_unmap() function is used to perform an unmapping
 *  operation on the kspace specified by the @a kspace argument using the 
 *  parameters encoded in the @a attr argument. This
 *  function invokes the L4_ProcessMapItem() kernel system call to perform
 *  the unmapping.
 *
 *  This function requires the following arguments:
 *
 *  @param kspace
 *    kspace on which the unmapping is performed.
 *
 *  @param attr
 *    Attribute encoding the unmapping parameters.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Unmapping was  successful.
 *
 *  @param other
 *    Return value from L4_ProcessMapItem().
 *
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation
 *  of the L4_ProcessMapItem() system call.
 *
 */
int okl4_kspace_unmap(okl4_kspace_t *kspace, okl4_kspace_unmap_attr_t *attr);

/**
 *  
 */
INLINE void okl4_kspace_attr_setprivileged(okl4_kspace_attr_t *attr,
        okl4_word_t privileged);

/**
 *  
 */
INLINE okl4_kspaceid_t okl4_kspace_getid(okl4_kspace_t *kspace);

/**
 *  The okl4_kspace_map_attr_init() function is used to initialize the
 *  kspace map attribute @a attr.  It must be invoked on the attribute
 *  prior to use in a map operation.
 *
 */
INLINE void okl4_kspace_map_attr_init(okl4_kspace_map_attr_t *attr);

/*
 *  Internal function to unmap the given virtual address range from
 *  kspace with the given id.
 *
 *  @param[in] kspace  The kspace to unmap pages from.
 *  @param[in] attr  Attributes containing information about the unmap
 *  operation.
 */
int _okl4_kspace_unmapbyid(okl4_kspaceid_t kspaceid,
        okl4_kspace_unmap_attr_t *attr);

/**
 *  The okl4_kspace_map_attr_setrange() function is used to encode a kspace
 *  map attribute @a attr with a virtual memory region @a range.  Mapping
 *  with this attribute will use @a range as the source virtual memory
 *  region.  
 * 
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.
 *
 *  @param range
 *    Virtual memory region to be encoded.
 *
 */
INLINE void okl4_kspace_map_attr_setrange(okl4_kspace_map_attr_t *attr,
        okl4_virtmem_item_t *range);

/**
 *  The okl4_kspace_map_attr_settarget() function is used to encode a
 *  kspace map attribute @a attr with a physical memory region @a target.
 *  Mapping with this attribute will use @a target as the target physical
 *  memory region.  
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.
 *
 *  @param target
 *    Target physical memory region to be encoded.
 *
 */
INLINE void okl4_kspace_map_attr_settarget(okl4_kspace_map_attr_t *attr,
        okl4_physmem_item_t *target);

/**
 *  The okl4_kspace_map_attr_setattributes() function is used to encode a
 *  kspace map attribute @a attr with a cache @a attribute. This influences
 *  the cache behaviour of the virtual-to-physical mapping.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.
 *
 *  @param attributes
 *    Cache attributes to be encoded.
 *
 */
INLINE void okl4_kspace_map_attr_setattributes(okl4_kspace_map_attr_t *attr,
        okl4_word_t attributes);

/**
 *  The okl4_kspace_map_attr_setperms() function is used to encode a kspace
 *  map attribute @a attr with the access permssions specified by the @a perms
 *  argument. The permissions provide access control to the virtual-to-physical mapping, that
 *  is, the read, write and execute permissions granted on the mapping.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encode.
 *
 *  @param perms
 *    Access control permissions to be encoded.
 *
 */
INLINE void okl4_kspace_map_attr_setperms(okl4_kspace_map_attr_t *attr,
        okl4_word_t perms);

/**
 *  The okl4_kspace_map_attr_setpagesize() function is used to encode a
 *  kspace map attribute @a attr with a native @a pagesize. This parameter
 *  determines the granularity at which the virtual-to-physical mapping
 *  will be made.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.
 *
 *  @param pagesize
 *    The page size granularity to be encoded.
 *
 */
INLINE void okl4_kspace_map_attr_setpagesize(okl4_kspace_map_attr_t *attr,
        okl4_word_t pagesize);

/**
 *  The okl4_kspace_unmap_attr_init() function is used to initialize the
 *  kspace unmap attribute @a attr.  This function must be invoked on the attribute
 *  prior to its use in an unmap operation.
 *
 */
INLINE void okl4_kspace_unmap_attr_init(okl4_kspace_unmap_attr_t *attr);

/**
 *  The okl4_kspace_unmap_attr_setrange() function is used to encode a
 *  kspace unmap attribute @a attr with a virtual memory region @a range.
 *  Unmapping with this attribute will remove the virtual-to-physical
 *  memory mapping at the virtual memory region, @a range.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.
 *
 *  @param range
 *    Virtual memory region to be encoded.
 *
 */
INLINE void okl4_kspace_unmap_attr_setrange(okl4_kspace_unmap_attr_t *attr,
        okl4_virtmem_item_t *range);

/**
 *  The okl4_kspace_unmap_attr_setpagesize() function is used to encode a
 *  kspace map attribute @a attr with a @a pagesize. This parameter
 *  determines the granularity at which the unmap operation will be made.
 *  This must match the page size used for the corresponding mapping
 *  operation that created the mapping in the first place.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.
 *
 *  @param pagesize
 *    Page size granularity to be encoded.
 *
 */
INLINE void okl4_kspace_unmap_attr_setpagesize(okl4_kspace_unmap_attr_t *attr,
        okl4_word_t pagesize);

#if defined(ARM_SHARED_DOMAINS)

/* Minimum alignment required for windows from one kspace to another. */
#define _OKL4_WINDOW_ALIGNMENT 0x100000

int _okl4_kspace_domain_share(okl4_kspaceid_t client,
        okl4_kspaceid_t share, int manage);

void _okl4_kspace_domain_unshare(okl4_kspaceid_t client,
        okl4_kspaceid_t share);

int _okl4_kspace_window_map(okl4_kspaceid_t client, okl4_kspaceid_t share,
        okl4_range_item_t window);

int _okl4_kspace_window_unmap(okl4_kspaceid_t client, okl4_kspaceid_t share,
        okl4_range_item_t window);

int _okl4_kspace_share_domain_add_window(okl4_kspaceid_t dest,
        okl4_kspaceid_t src, int manager, okl4_virtmem_item_t window);

#endif

/* Inlined Methods */

INLINE void
okl4_kspace_attr_init(okl4_kspace_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_KSPACE_ATTR);
    attr->id.space_no = 0;
    attr->kclist = NULL;
    attr->utcb_area = NULL;
    attr->privileged = 0;
}

INLINE void
okl4_kspace_attr_setkclist(okl4_kspace_attr_t *attr,
        okl4_kclist_t *kclist)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KSPACE_ATTR);

    attr->kclist = kclist;
}

INLINE void
okl4_kspace_attr_setid(okl4_kspace_attr_t *attr,
        okl4_kspaceid_t kspaceid)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KSPACE_ATTR);

    attr->id = kspaceid;
}

INLINE void
okl4_kspace_attr_setutcbarea(okl4_kspace_attr_t *attr,
        okl4_utcb_area_t *utcb_area)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KSPACE_ATTR);

    attr->utcb_area = utcb_area;
}

INLINE void
okl4_kspace_attr_setprivileged(okl4_kspace_attr_t *attr,
        okl4_word_t privileged)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KSPACE_ATTR);

    attr->privileged = privileged;
}

INLINE okl4_kspaceid_t
okl4_kspace_getid(okl4_kspace_t *kspace)
{
    assert(kspace != NULL);
    return kspace->id;
}

INLINE void
okl4_kspace_map_attr_init(okl4_kspace_map_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_KSPACE_MAP_ATTR);
    attr->range = NULL;
    attr->target = NULL;
    attr->perms = L4_Readable | L4_Writable;
    attr->attributes = L4_DefaultMemory;
    attr->page_size = OKL4_DEFAULT_PAGESIZE;
}

INLINE void
okl4_kspace_map_attr_setrange(okl4_kspace_map_attr_t *attr,
        okl4_virtmem_item_t *range)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KSPACE_MAP_ATTR);

    attr->range = range;
}

INLINE void
okl4_kspace_map_attr_settarget(okl4_kspace_map_attr_t *attr,
        okl4_physmem_item_t *target)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KSPACE_MAP_ATTR);

    attr->target = target;
}

INLINE void
okl4_kspace_map_attr_setattributes(okl4_kspace_map_attr_t *attr,
        okl4_word_t attributes)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KSPACE_MAP_ATTR);

    attr->attributes = attributes;
}

INLINE void
okl4_kspace_map_attr_setperms(okl4_kspace_map_attr_t *attr,
        okl4_word_t perms)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KSPACE_MAP_ATTR);

    attr->perms = perms;
}

INLINE void
okl4_kspace_map_attr_setpagesize(okl4_kspace_map_attr_t *attr,
        okl4_word_t pagesize)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KSPACE_MAP_ATTR);

    attr->page_size = pagesize;
}

INLINE void
okl4_kspace_unmap_attr_init(okl4_kspace_unmap_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_KSPACE_UNMAP_ATTR);
    attr->range = NULL;
    attr->page_size = OKL4_DEFAULT_PAGESIZE;
}

INLINE void
okl4_kspace_unmap_attr_setrange(okl4_kspace_unmap_attr_t *attr,
        okl4_virtmem_item_t *range)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KSPACE_UNMAP_ATTR);

    attr->range = range;
}

INLINE void
okl4_kspace_unmap_attr_setpagesize(okl4_kspace_unmap_attr_t *attr,
        okl4_word_t pagesize)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KSPACE_UNMAP_ATTR);

    attr->page_size = pagesize;
}


#endif /* !__OKL4__KSPACE_H__ */
